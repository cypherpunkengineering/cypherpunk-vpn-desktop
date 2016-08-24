#include "config.h"
#include "daemon.h"
#include "openvpn.h"
#include "logger.h"

#include <thread>
#include <asio.hpp>
#include <jsonrpc-lean/server.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

CypherDaemon* g_daemon = nullptr;

using namespace std::placeholders;


CypherDaemon::CypherDaemon()
	: _rpc_client(_json_handler)
	, _process(nullptr)
	, _next_process(nullptr)
	, _state(STARTING)
{

}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon built on " __TIMESTAMP__;

	_ws_server.set_message_handler(std::bind(&CypherDaemon::OnReceiveMessage, this, _1, _2));
	_ws_server.set_open_handler([this](Connection c) {
		bool first = _connections.empty();
		_connections.insert(c);
		if (first) OnFirstClientConnected();
	});
	_ws_server.set_close_handler([this](Connection c) {
		_connections.erase(c);
		if (_connections.empty()) OnLastClientDisconnected();
	});

	{
		using namespace websocketpp::log;
		_ws_server.clear_access_channels(alevel::all);
		_ws_server.clear_error_channels(elevel::all);
		_ws_server.set_error_channels(elevel::fatal | elevel::rerror | elevel::warn);
#ifdef _DEBUG
		_ws_server.set_access_channels(alevel::access_core);
		_ws_server.set_error_channels(elevel::info | elevel::library);
#endif
	}

	_ws_server.init_asio();
	_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 9337));
	_ws_server.start_accept();

	_rpc_server.RegisterFormatHandler(_json_handler);
	{
		auto& d = _rpc_server.GetDispatcher();
		d.AddMethod("connect", &CypherDaemon::RPC_connect, *this);
		d.AddMethod("disconnect", &CypherDaemon::RPC_disconnect, *this);
	}

	_state = INITIALIZED;
	_ws_server.run();

	return 0;
}

void CypherDaemon::RequestShutdown()
{
	if (!_ws_server.stopped())
		_ws_server.stop();
}

void CypherDaemon::SendToClient(Connection connection, const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	_ws_server.send(connection, data->GetData(), data->GetSize(), websocketpp::frame::opcode::TEXT);
}

void CypherDaemon::SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	for (const auto& it : _connections)
		SendToClient(it, data);
}

void CypherDaemon::OnFirstClientConnected()
{

}

void CypherDaemon::OnLastClientDisconnected()
{

}

void CypherDaemon::OnReceiveMessage(Connection connection, WebSocketServer::message_ptr msg)
{
	try
	{
		auto response = _rpc_client.ParseResponse(msg->get_payload());
	}
	catch (const jsonrpc::Fault&)
	{
		auto result = _rpc_server.HandleRequest(msg->get_payload());
		SendToClient(connection, result);
	}
}

int CypherDaemon::GetAvailablePort(int hint)
{
	return hint;
}

void WriteOpenVPNProfile(std::ostream& out, const Settings::Connection& connection)
{
	using namespace std;

	// Generic parameters for all server endpoints
	out << "client" << endl;
	out << "nobind" << endl;
	out << "dev tun" << endl;
	out << "resolv-retry infinite" << endl;

	// Server endpoint specific parameters
	out << "proto ";
	if (connection.protocol.empty())
		out << "udp" << endl;
	else
		out << connection.protocol << endl;
	out << "remote " << connection.remoteIP << ' ' << connection.remotePort << endl;
	if (connection.mtu != 0)
		out << "tun-mtu " + connection.mtu << endl;
	out << "cipher " << connection.cipher << endl;

	// Depending on routing settings
	out << "redirect-gateway def1" << endl;
	//out << "route IP MASK GW METRIC" << endl;
	//out << "route-delay 0" << endl;

	if (!connection.certificateAuthority.empty())
	{
		out << "<ca>" << endl;
		if (connection.certificateAuthority.front() != "-----BEGIN CERTIFICATE-----")
			out << "-----BEGIN CERTIFICATE-----" << endl;
		for (const auto& line : connection.certificateAuthority)
			out << line << endl;
		if (connection.certificateAuthority.back() != "-----END CERTIFICATE-----")
			out << "-----END CERTIFICATE-----" << endl;
		out << "</ca>" << endl;
	}
	if (!connection.certificate.empty())
	{
		out << "<cert>" << endl;
		if (connection.certificate.front() != "-----BEGIN CERTIFICATE-----")
			out << "-----BEGIN CERTIFICATE-----" << endl;
		for (const auto& line : connection.certificate)
			out << line << endl;
		if (connection.certificate.back() != "-----END CERTIFICATE-----")
			out << "-----END CERTIFICATE-----" << endl;
		out << "</cert>" << endl;
	}
	if (!connection.privateKey.empty())
	{
		out << "<key>" << endl;
		if (connection.privateKey.front() != "-----BEGIN PRIVATE KEY-----")
			out << "-----BEGIN PRIVATE KEY-----" << endl;
		for (const auto& line : connection.privateKey)
			out << line << endl;
		if (connection.privateKey.back() != "-----END PRIVATE KEY-----")
			out << "-----END PRIVATE KEY-----" << endl;
		out << "</key>" << endl;
	}
}

bool CypherDaemon::RPC_connect(const jsonrpc::Value::Struct& params)
{
	if (_process)
		return false;

	const auto& lines = params.at("profile").AsArray();
	int index = 0;

	auto vpn = CreateOpenVPNProcess(_ws_server.get_io_service());
	std::vector<std::string> args;

	int port = GetAvailablePort(DEFAULT_OPENVPN_PORT_BASE + index);
	args.push_back("--management");
	args.push_back("127.0.0.1");
	args.push_back(std::to_string(port));
	args.push_back("--management-hold");

	args.push_back("--dev-node");
	args.push_back(GetAvailableAdapter(index));

	args.push_back("--config");
	args.push_back("cypher.ovpn");

	vpn->Run(args);
	vpn->StartManagementInterface(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port));

	vpn->OnManagementResponse("HOLD", [=](const std::string& line) {
		vpn->SendManagementCommand("\nhold release\n");
	});
	vpn->OnManagementResponse("STATE", [=](const std::string& line) {
		std::deque<jsonrpc::Value> params;
		size_t pos, last = 0;
		while ((pos = line.find(',', last)) != line.npos)
		{
			params.emplace_back(line.substr(last, pos - last));
			last = pos + 1;
		}
		if (last != line.size())
		{
			params.emplace_back(line.substr(last));
		}
		if (params.size() >= 2 && params.at(1).AsString() == "EXITING")
		{
			_process = nullptr;
		}
		SendToAllClients(_rpc_client.BuildNotificationData("state", params));
	});
	vpn->OnManagementResponse("BYTECOUNT", [=](const std::string& line) {
		std::deque<jsonrpc::Value> params;
		size_t pos, last = 0;
		while ((pos = line.find(',', last)) != line.npos)
		{
			params.emplace_back(line.substr(last, pos - last));
			last = pos + 1;
		}
		if (last != line.size())
		{
			params.emplace_back(line.substr(last));
		}
		SendToAllClients(_rpc_client.BuildNotificationData("bytecount", params));
	});

	vpn->SendManagementCommand("\nstate on\nbytecount 5\nhold release\n");

	//vpn->SendManagementCommand("signal SIGTERM\n");
	//vpn->SendManagementCommand("exit\n");

	//vpn->StopManagementInterface();
	//delete vpn;

	_process = vpn;

	return true;
}

void CypherDaemon::RPC_disconnect()
{
	if (_process)
	{
		_process->SendManagementCommand("\nsignal SIGTERM\n");
		// FIXME: Shut down asynchronously
		//delete _process;
		//_process = nullptr;
	}
}
