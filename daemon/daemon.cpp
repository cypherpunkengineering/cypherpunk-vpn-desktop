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
#ifdef WIN32
#include <direct.h> // FIXME: temporary
#else
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif


CypherDaemon* g_daemon = nullptr;

using namespace std::placeholders;


CypherDaemon::CypherDaemon()
	: _rpc_client(_json_handler)
	, _process(nullptr)
	, _next_process(nullptr)
	, _state(STARTING)
	, _firewallMode(Disabled)
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
	_ws_server.set_reuse_addr(true);
	_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 9337));
	_ws_server.start_accept();

	_rpc_server.RegisterFormatHandler(_json_handler);
	{
		auto& d = _rpc_server.GetDispatcher();
		d.AddMethod("getState", &CypherDaemon::RPC_getState, *this);
		d.AddMethod("connect", &CypherDaemon::RPC_connect, *this);
		d.AddMethod("disconnect", &CypherDaemon::RPC_disconnect, *this);
		d.AddMethod("setFirewall", &CypherDaemon::RPC_setFirewall, *this);
		d.AddMethod("ping", [](){});
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

void CypherDaemon::OnOpenVPNProcessExited(OpenVPNProcess* process)
{
	if (process == _process)
	{
		_process = nullptr;
		if (_state != DISCONNECTED)
		{
			_state = DISCONNECTED;
			OnStateChanged();
		}
	}
}

int CypherDaemon::GetAvailablePort(int hint)
{
	return hint;
}

static inline const char* GetStateString(CypherDaemon::State state)
{
	switch (state)
	{
#define CASE(n) case CypherDaemon::n: return #n;
	CASE(STARTING)
	CASE(DISCONNECTED)
	CASE(CONNECTING)
	CASE(CONNECTED)
	CASE(SWITCHING)
	CASE(DISCONNECTING)
	CASE(EXITING)
	CASE(EXITED)
#undef CASE
	}
	return "";
}

void CypherDaemon::OnStateChanged()
{
	jsonrpc::Value::Struct params;
	params["state"] = GetStateString(_state);
	if (_state == CONNECTED)
	{
		params["localIP"] = _localIP;
		params["remoteIP"] = _remoteIP;
		params["bytesReceived"] = _bytesReceived;
		params["bytesSent"] = _bytesSent;
	}
	SendToAllClients(_rpc_client.BuildNotificationData("state", params));
}

jsonrpc::Value::Struct CypherDaemon::RPC_getState()
{
	jsonrpc::Value::Struct result;
	result.insert(std::make_pair("state", jsonrpc::Value(GetStateString(_state))));
	return std::move(result);
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

template<typename CB>
static inline size_t SplitToIterators(const std::string& text, char sep, const CB& cb)
{
	size_t pos, last = 0, count = 0;
	auto begin = text.cbegin();
	while ((pos = text.find(sep, last)) != text.npos)
	{
		cb(begin + last, begin + pos);
		last = pos + 1;
		count++;
	}
	if (last != text.size())
	{
		cb(begin + last, text.cend());
		count++;
	}
	return count;
}

template<typename CB>
static inline size_t SplitToStrings(const std::string& text, char sep, const CB& cb)
{
	return SplitToIterators(text, sep, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { return std::string(b, e); });
}

static inline std::vector<std::string> SplitToVector(const std::string& text, char sep)
{
	std::vector<std::string> result;
	SplitToIterators(text, sep, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { result.emplace_back(b, e); });
	return std::move(result);
}

bool CypherDaemon::RPC_connect(const jsonrpc::Value::Struct& params)
{
	Settings::Connection c(params);

	if (_state == CONNECTED)
	{
		// Check if we're being asked to connect to the same place
		return _process->IsSameServer(c);
		// TODO: Support switching to different server
	}
	else if (_state != State::DISCONNECTED)
	{
		// Already busy with something else; reject request
		return false;
	}

	_bytesReceived = 0;
	_bytesSent = 0;
	_state = CONNECTING;
	OnStateChanged();

	static int index = 0;
	index++; // FIXME: Just make GetAvailablePort etc. work properly instead

	auto vpn = CreateOpenVPNProcess(_ws_server.get_io_service());

	int port = vpn->StartManagementInterface();

	_process = vpn;

	std::vector<std::string> args;

	args.push_back("--management");
	args.push_back("127.0.0.1");
	args.push_back(std::to_string(port));
	args.push_back("--management-hold");
	args.push_back("--management-client");

	args.push_back("--verb");
	args.push_back("4");

#if OS_WIN
	args.push_back("--dev-node");
	args.push_back(GetAvailableAdapter(index));
#endif

#if OS_OSX
	args.push_back("--script-security");
	args.push_back("2");

	args.push_back("--up");
	args.push_back(GetPath(ScriptsDir, "up.sh") + " -9 -d -f -m -w -ptADGNWradsgnw");
	args.push_back("--down");
	args.push_back(GetPath(ScriptsDir, "down.sh") + " -9 -d -f -m -w -ptADGNWradsgnw");
	args.push_back("--route-pre-down");
	args.push_back(GetPath(ScriptsDir, "route-pre-down.sh"));
	args.push_back("--tls-verify");
	args.push_back(GetPath(ScriptsDir, "tls-verify.sh"));
	args.push_back("--ipchange");
	args.push_back(GetPath(ScriptsDir, "ipchange.sh"));
	args.push_back("--route-up");
	args.push_back(GetPath(ScriptsDir, "route-up.sh"));
#else
	args.push_back("--script-security");
	args.push_back("1");
#endif

	args.push_back("--config");

#if OS_WIN
	char profile_basename[32];
	snprintf(profile_basename, sizeof(profile_basename), "profile%d.ovpn", index);
	_mkdir(GetPath(ProfileDir).c_str());
	std::string profile_filename = GetPath(ProfileDir, profile_basename);
#else
	std::string profile_filename = "/tmp/profile.XXXXXX";
	mktemp(&profile_filename[0]);
#endif
	{
		std::ofstream f(profile_filename.c_str());
		WriteOpenVPNProfile(f, c);
		f.close();
	}
	args.push_back(profile_filename);

	vpn->OnManagementResponse("HOLD", [=](const std::string& line) {
		vpn->SendManagementCommand("\nhold release\n");
	});
	vpn->OnManagementResponse("STATE", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			if (params.size() >= 2)
			{
				const auto& s = params.at(1);
				if (s == "CONNECTED")
				{
					if (_state == DISCONNECTING)
					{
						_process->SendManagementCommand("\nsignal SIGTERM\n");
						return;
					}
					if (params.size() >= 5)
					{
						_localIP = params.at(3);
						_remoteIP = params.at(4);
					}
					_state = CONNECTED;
					OnStateChanged();
				}
				else if (s == "RECONNECTING")
				{
					if (_state == CONNECTED)
					{
						_state = CONNECTING;
						OnStateChanged();
					}
				}
				else if (s == "EXITING")
				{
					_process = nullptr;
					_state = DISCONNECTED;
					OnStateChanged();
				}
			}
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << e;
			_state = DISCONNECTED;
			if (_process)
			{
				_process->Kill();
				_process = nullptr;
			}
		}
	});
	vpn->OnManagementResponse("BYTECOUNT", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			_bytesReceived = std::stoll(params[0]);
			_bytesSent = std::stoll(params[1]);
			OnStateChanged();
		}
		catch (const std::exception& e) { LOG(WARNING) << e; }
	});

	vpn->Run(args);

	vpn->SendManagementCommand("\nstate on\nbytecount 5\nhold release\n");

	//vpn->SendManagementCommand("signal SIGTERM\n");
	//vpn->SendManagementCommand("exit\n");

	//vpn->StopManagementInterface();
	//delete vpn;

	return true;
}

void CypherDaemon::RPC_disconnect()
{
	if (_state == CONNECTING || _state == CONNECTED)
	{
		_state = DISCONNECTING;
		OnStateChanged();
		_process->SendManagementCommand("\nsignal SIGTERM\n");
	}
}

template<typename T>
static inline const T& GetMember(const jsonrpc::Value::Struct& obj, const char* name, const T& default_value)
{
	try
	{
		return obj.at(name).AsType<T>();
	}
	catch (...)
	{
		return default_value;
	}
}

bool CypherDaemon::RPC_setFirewall(const jsonrpc::Value::Struct& params)
{
	FirewallFlags flags = Nothing;

	auto mode = GetMember<std::string>(params, "mode", "off");

	if (mode == "on")
		SetFirewallSettings(true);
	else
		SetFirewallSettings(false);

	//if (mode == "on")
	//	flags |= BlockIPv6;
	//else if (mode == "auto")
	//else /* mode == "off" */

	//if (GetMember<bool>(params, "blockIPv6", false))
	//	flags |= BlockIPv6;

	return true;
}
