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
	g_settings.ReadFromDisk();
}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon built on " __TIMESTAMP__;

	_ws_server.set_message_handler(std::bind(&CypherDaemon::OnReceiveMessage, this, _1, _2));
	_ws_server.set_open_handler([this](Connection c) {
		bool first = _connections.empty();
		_connections.insert(c);
		if (first) OnFirstClientConnected();
		OnClientConnected(c);
	});
	_ws_server.set_close_handler([this](Connection c) {
		_connections.erase(c);
		OnClientDisconnected(c);
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

	{
		auto& d = _dispatcher;
		d.AddMethod("get", &CypherDaemon::RPC_get, *this);
		d.AddMethod("connect", &CypherDaemon::RPC_connect, *this);
		d.AddMethod("disconnect", &CypherDaemon::RPC_disconnect, *this);
		d.AddMethod("applySettings", &CypherDaemon::RPC_applySettings, *this);
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

void CypherDaemon::OnClientConnected(Connection c)
{

}

void CypherDaemon::OnClientDisconnected(Connection c)
{

}

void CypherDaemon::OnLastClientDisconnected()
{

}

void CypherDaemon::OnReceiveMessage(Connection connection, WebSocketServer::message_ptr msg)
{
	CHECK_THREAD();
	LOG(VERBOSE) << "Received RPC message: " << msg->get_payload();
	try
	{
		auto response = _json_handler.CreateReader(msg->get_payload())->GetResponse();
		return; // this was a response to a previous server->client call; our work is done
	}
	catch (const jsonrpc::Fault&) {}

	auto writer = _json_handler.CreateWriter();
	try
	{
		auto request = _json_handler.CreateReader(msg->get_payload())->GetRequest();
		auto response = _dispatcher.Invoke(request.GetMethodName(), request.GetParameters(), request.GetId());
		try
		{
			response.ThrowIfFault();
			// Special case: the 'get' method responds with a separate notification
			if (request.GetMethodName() == "get")
			{
				SendToClient(connection, _rpc_client.BuildNotificationData(request.GetParameters().front().AsString(), response.GetResult()));
				return;
			}
			if (response.GetId().IsBoolean() && response.GetId().AsBoolean() == false)
				return; // this was a notification; no response is needed
		}
		catch (const jsonrpc::Fault& e)
		{
			LOG(ERROR) << "RPC error: " << e;
		}
		response.Write(*writer);
	}
	catch (const jsonrpc::Fault& e)
	{
		LOG(ERROR) << "RPC error: " << e;
		jsonrpc::Response(e.GetCode(), e.GetString(), jsonrpc::Value()).Write(*writer);
	}
	auto data = writer->GetData();
	if (data->GetSize() > 0)
		SendToClient(connection, data);
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

static inline JsonObject FilterJsonObject(const JsonObject& obj, const std::vector<std::string>& keys)
{
	JsonObject result;
	for (auto& s : keys)
	{
		auto it = obj.find(s);
		if (it != obj.end())
			result.insert(std::make_pair(s, it->second));
	}
	return std::move(result);
}

static inline JsonObject FilterJsonObject(JsonObject&& obj, const std::vector<std::string>& keys)
{
	JsonObject result;
	for (auto& s : keys)
	{
		auto it = obj.find(s);
		if (it != obj.end())
			result.insert(std::make_pair(s, std::move(it->second)));
	}
	return std::move(result);
}

JsonObject CypherDaemon::MakeStateObject()
{
	JsonObject state;
	state["state"] = GetStateString(_state);
	if (_state == CONNECTED)
	{
		state["localIP"] = _localIP;
		state["remoteIP"] = _remoteIP;
		state["bytesReceived"] = _bytesReceived;
		state["bytesSent"] = _bytesSent;
	}
	return std::move(state);
}

JsonObject CypherDaemon::MakeConfigObject()
{
	JsonObject config;
	config["servers"] = std::vector<JsonObject> {
		{ { "remote", "208.111.52.1 7133" }, { "country", "jp" }, { "name", "Tokyo 1, Japan" } },
		{ { "remote", "208.111.52.2 7133" }, { "country", "jp" }, { "name", "Tokyo 2, Japan" } },
		{ { "remote", "199.68.252.203 7133" }, { "country", "us" }, { "name", "Honolulu, HI, USA" } },
	};
	return std::move(config);
}

void CypherDaemon::OnStateChanged()
{
	// TODO: Only call if firewall-related state has changed
	ApplyFirewallSettings();
	SendToAllClients(_rpc_client.BuildNotificationData("state", MakeStateObject()));
}

void CypherDaemon::OnSettingsChanged(const std::vector<std::string>& names)
{

}

JsonObject CypherDaemon::RPC_get(const std::string& type)
{
	if (type == "state")
		return MakeStateObject();
	if (type == "settings")
		return g_settings.map();
	if (type == "config")
		return MakeConfigObject();
	throw jsonrpc::InvalidParametersFault();
}

void CypherDaemon::RPC_applySettings(const JsonObject& settings)
{
	std::vector<std::string> changed;
	for (auto& p : settings)
	{
		g_settings[p.first] = JsonValue(p.second);
	}
}

void WriteOpenVPNProfile(std::ostream& out, const JsonObject& settings)
{
	using namespace std;

	std::map<std::string, std::string> config = {
		{ "client", "" },
		{ "nobind", "" },
		{ "dev", "tun" },
		{ "proto", "udp" },
		{ "tun-mtu", "1400" },
		{ "fragment", "1300" },
		{ "mssfix", "1200" },
		{ "ping", "10" },
		{ "ping-exit", "60" },
		{ "resolv-retry", "infinite" },
		{ "cipher", "AES-128-CBC" },
		{ "redirect-gateway", "def1" },
		{ "route-delay", "0" },
	};

	for (const auto& e : settings)
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
		default:
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
		auto it = settings.find(t.first);
		if (it != settings.end() && it->second.IsArray())
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

bool CypherDaemon::RPC_connect()
{
	// FIXME: Shouldn't simply read raw profile parameters from the params
	const JsonObject& settings = g_settings.map();

	if (_state == CONNECTED)
	{
		// Check if we're being asked to connect to the same place
		return _process->IsSameServer(settings);
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
	args.push_back(GetPath(ScriptsDir, "up.sh") + " -9 -d -f -m -w -pradsgnwADSGNW");
	args.push_back("--down");
	args.push_back(GetPath(ScriptsDir, "down.sh") + " -9 -d -f -m -w -pradsgnwADSGNW");
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
		WriteOpenVPNProfile(f, settings);
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

/*
bool CypherDaemon::RPC_setFirewall(const jsonrpc::Value::Struct& params)
{
	FirewallFlags flags = Nothing;

	auto mode = GetMember<std::string>(params, "mode", "off");
	auto allowLAN = GetMember<bool>(params, "allowLAN", true);
	if (mode != g_settings.killswitchMode() || allowLAN != g_settings.allowLAN())
	{
		g_settings.killswitchMode(mode);
		g_settings.allowLAN(allowLAN);
		g_settings.OnChanged();
		ApplyFirewallSettings();
	}

	return true;
}
*/
