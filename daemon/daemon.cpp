#include "config.h"
#include "daemon.h"
#include "openvpn.h"
#include "logger.h"
#include "path.h"

#include <thread>
#include <asio.hpp>
#include <jsonrpc-lean/server.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <direct.h> // FIXME: temporary

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
	// FIXME: Cleanly shut down all OpenVPN connections, then exit
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

	std::map<std::string, std::string> config = {
		{ "client", "" },
		{ "nobind", "" },
		{ "dev", "tun" },
		{ "proto", "udp" },
		{ "tun-mtu", "1400" },
		//{ "fragment", "1300" },
		{ "mssfix", "1200" },
		{ "ping", "10" },
		{ "ping-exit", "60" },
		{ "resolv-retry", "infinite" },
		{ "cipher", "AES-128-CBC" },
		//{ "redirect-gateway", "def1" },
		{ "route-delay", "0" },
	};

	for (const auto& e : connection)
	{
		switch (e.second.GetType())
		{
		case jsonrpc::Value::Type::BOOLEAN:
			config[e.first] = e.second.AsBoolean() ? "true" : "false";
			break;
		case jsonrpc::Value::Type::DOUBLE:
			config[e.first] = std::to_string(e.second.AsDouble());
			break;
		case jsonrpc::Value::Type::INTEGER_32:
			config[e.first] = std::to_string(e.second.AsInteger32());
			break;
		case jsonrpc::Value::Type::INTEGER_64:
			config[e.first] = std::to_string(e.second.AsInteger64());
			break;
		case jsonrpc::Value::Type::NIL:
			config[e.first] = "";
			break;
		case jsonrpc::Value::Type::STRING:
			config[e.first] = e.second.AsString();
			break;
		}
	}

	for (const auto& e : config)
	{
		out << e.first;
		if (!e.second.empty())
			out << ' ' << e.second;
		out << endl;
	}

	static const std::map<std::string, std::string> arrayTypes = {
		{ "ca", "CERTIFICATE" },
		{ "cert", "CERTIFICATE" },
		{ "key", "PRIVATE KEY" },
	};
	static const std::string dash = "-----";

	for (const auto& t : arrayTypes)
	{
		auto it = connection.find(t.first);
		if (it != connection.end() && it->second.IsArray())
		{
			const auto& lines = it->second.AsArray();
			auto ensure = [&](const jsonrpc::Value& value, const std::string& str) {
				if (value.AsString() != str) out << str << endl;
			};
			if (!lines.empty())
			{
				out << '<' << t.first << '>' << endl;
				ensure(lines.front(), dash + "BEGIN " + t.second + dash);
				for (const auto& line : lines)
					out << line.AsString() << endl;
				ensure(lines.front(), dash + "END " + t.second + dash);
				out << '<' << '/' << t.first << '>' << endl;
			}
		}
	}
}

bool CypherDaemon::RPC_connect(const jsonrpc::Value::Struct& params)
{
	Settings::Connection c(params);

	if (_state == CONNECTED)
	{
		// Check if we're being asked to connect to the same place
		if (_process->IsSameServer(c))
			return true;
	}
	else if (_state != State::DISCONNECTED)
	{
		// Already busy with something else; reject request
		return false;
	}

	static int index = 0;
	index++; // FIXME: Just make GetAvailablePort etc. work properly instead

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

	char profile_basename[32];
	snprintf(profile_basename, sizeof(profile_basename), "profile%d.ovpn", index);
	mkdir(GetPath(ProfileDir).c_str());
	std::string profile_filename = GetPath(ProfileDir, profile_basename);
	{
		std::ofstream f(profile_filename.c_str());
		WriteOpenVPNProfile(f, c);
		f.close();
	}
	args.push_back(profile_filename);

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
