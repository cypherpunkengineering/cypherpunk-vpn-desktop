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

	{
		_rpc_server.RegisterFormatHandler(_json_handler);
		{
			auto& d = _rpc_server.GetDispatcher();
			d.AddMethod("test", [](int a, std::string b, double c) {
				return "result";
			});
			d.AddMethod("disconnect", []() {
				//_process->
			});
		}

		_ws_server.run();
	}

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
