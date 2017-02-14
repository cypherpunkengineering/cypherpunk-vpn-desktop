#include "config.h"
#include "daemon.h"
#include "openvpn.h"
#include "logger.h"
#include "ping.h"
#include "path.h"
#include "version.h"

#include <chrono>
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

unsigned short ServerPingerThinger::_global_sequence_number = 0;

using namespace std::placeholders;


CypherDaemon::CypherDaemon()
	: _rpc_client(_json_handler)
	, _state(STARTING)
	, _shouldConnect(false)
	, _needsReconnect(false)
	, _notify_scheduled(false)
{
	g_config.SetOnChangedHandler(THIS_CALLBACK(OnConfigChanged));
	g_account.SetOnChangedHandler(THIS_CALLBACK(OnAccountChanged));
	g_settings.SetOnChangedHandler(THIS_CALLBACK(OnSettingsChanged));

	//_state.SetOnChangedHandler(THIS_CALLBACK(OnStateChanged));
}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon version v" VERSION " built on " __TIMESTAMP__;

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

	_ws_server.init_asio(&_io);
	_ws_server.set_reuse_addr(true);
	_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 9337));
	_ws_server.start_accept();

	{
		auto& d = _dispatcher;
		d.AddMethod("get", &CypherDaemon::RPC_get, *this);
		d.AddMethod("connect", &CypherDaemon::RPC_connect, *this);
		d.AddMethod("disconnect", &CypherDaemon::RPC_disconnect, *this);
		d.AddMethod("setAccount", &CypherDaemon::RPC_setAccount, *this);
		d.AddMethod("applyConfig", &CypherDaemon::RPC_applyConfig, *this);
		d.AddMethod("applySettings", &CypherDaemon::RPC_applySettings, *this);
		d.AddMethod("ping", [](){});
	}

	_state = INITIALIZED;

	g_settings.ReadFromDisk();

	_ws_server.run(); // internally calls _io.run()

	return 0;
}

void CypherDaemon::RequestShutdown()
{
	_io.post([this]() {
		if (_process)
		{
			try
			{
				// Shut down any OpenVPN process first.
				_process->Shutdown();
				_process->AsyncWait([this](const asio::error_code& error) {
					LOG(INFO) << "Stopping message loop";
					if (!_ws_server.stopped())
						_ws_server.stop();
				});
				return;
			}
			catch (const SystemException& e)
			{
				LOG(ERROR) << e;
			}
		}
		// Either no process is running, or something went wrong above.
		if (!_ws_server.stopped())
			_ws_server.stop();
	});
}

void CypherDaemon::SendToClient(Connection connection, const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	LOG(VERBOSE) << "Sending RPC message: " << std::string(data->GetData(), data->GetSize());
	_ws_server.send(connection, data->GetData(), data->GetSize(), websocketpp::frame::opcode::TEXT);
}

void CypherDaemon::SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	for (const auto& it : _connections)
		SendToClient(it, data);
}

void CypherDaemon::SendErrorToAllClients(const std::string& name, const std::string& description)
{
	SendToAllClients(_rpc_client.BuildNotificationData("error", name, description));
}

void CypherDaemon::OnFirstClientConnected()
{
	// If the kill-switch is set to always on, trigger it when the first client connects
	if (g_settings.firewall() == "on")
		ApplyFirewallSettings();
}

void CypherDaemon::OnClientConnected(Connection c)
{
	SendToClient(c, _rpc_client.BuildNotificationData("config", MakeConfigObject()));
	SendToClient(c, _rpc_client.BuildNotificationData("account", MakeAccountObject()));
	SendToClient(c, _rpc_client.BuildNotificationData("settings", g_settings.map()));
	SendToClient(c, _rpc_client.BuildNotificationData("state", MakeStateObject()));
}

void CypherDaemon::OnClientDisconnected(Connection c)
{

}

void CypherDaemon::OnLastClientDisconnected()
{
	switch (_state)
	{
		case CONNECTED:
		case CONNECTING:
		case SWITCHING:
			_state = DISCONNECTING;
			OnStateChanged(STATE);
			_process->SendManagementCommand("signal SIGTERM");
			break;
		default:
			// If the kill-switch is set to always on, trigger it here so it can be disabled when
			// the last client disconnects (this also happens above inside OnStateChanged(STATE))
			if (g_settings.firewall() == "on")
				ApplyFirewallSettings();
			break;
	}
}

void CypherDaemon::OnReceiveMessage(Connection connection, WebSocketServer::message_ptr msg)
{
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

void CypherDaemon::OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	//if (process == _process.get())
	{
		if (!error)
		{
			if (line.compare(0, 14, "****** DAEMON ") == 0)
			{
				line.erase(0, 14);
				OnOpenVPNCallback(process, std::move(line));
			}
			else
			{
				LOG_EX(LogLevel::VERBOSE, true, Location("OpenVPN:STDOUT")) << line;
			}
		}
		else
			LOG(WARNING) << "OpenVPN:STDOUT error: " << error;
	}
}

void CypherDaemon::OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	//if (process == _process.get())
	{
		if (!error)
		{
			LOG_EX(LogLevel::WARNING, true, Location("OpenVPN:STDERR")) << line;
		}
		else
			LOG(WARNING) << "OpenVPN:STDERR error: " << error;
	}
}

void CypherDaemon::OnOpenVPNCallback(OpenVPNProcess* process, std::string args)
{
	LOG(INFO) << "DAEMON CALLBACK: " << args;
}

void CypherDaemon::OnOpenVPNProcessExited(OpenVPNProcess* process)
{
	if (process == _process.get())
	{
		_process.reset();
		if (_state == SWITCHING)
		{
			_io.post([this](){ DoConnect(); });
		}
		else if (_state != DISCONNECTED)
		{
			_state = DISCONNECTED;
			_needsReconnect = false;
			OnStateChanged(STATE | NEEDSRECONNECT);
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

static inline JsonObject FilterJsonObject(const JsonObject& obj, const std::unordered_set<std::string>* keys)
{
	if (keys)
	{
		JsonObject result;
		for (auto& s : *keys)
		{
			auto it = obj.find(s);
			if (it != obj.end())
				result.insert(std::make_pair(it->first, it->second));
		}
		return result;
	}
	return obj;
}
static inline JsonObject FilterJsonObject(JsonObject&& obj, const std::unordered_set<std::string>* keys)
{
	if (keys)
	{
		JsonObject result;
		for (auto& s : *keys)
		{
			auto it = obj.find(s);
			if (it != obj.end())
				result.insert(std::make_pair(it->first, std::move(it->second)));
		}
		return result;
	}
	return obj;
}


void CypherDaemon::OnConfigChanged(const char* key)
{
	_to_notify.config.insert(key);
	if (!_notify_scheduled)
	{
		_notify_scheduled = true;
		_io.post(THIS_CALLBACK(NotifyChanges));
	}
}
void CypherDaemon::OnAccountChanged(const char* key)
{
	_to_notify.account.insert(key);
	if (!_notify_scheduled)
	{
		_notify_scheduled = true;
		_io.post(THIS_CALLBACK(NotifyChanges));
	}
}
void CypherDaemon::OnSettingsChanged(const char* key)
{
	_to_notify.settings.insert(key);
	if (!_notify_scheduled)
	{
		_notify_scheduled = true;
		_io.post(THIS_CALLBACK(NotifyChanges));
	}
}

void CypherDaemon::OnStateChanged(unsigned int flags)
{
	if ((_to_notify.state & flags) != flags)
	{
		_to_notify.state |= flags;
		if (!_notify_scheduled)
		{
			_notify_scheduled = true;
			_io.post(THIS_CALLBACK(NotifyChanges));
		}
	}
}


void CypherDaemon::NotifyChanges()
{
	if (!_notify_scheduled) return;

	_notify_scheduled = false;
	std::unordered_set<std::string> config = std::move(_to_notify.config), account = std::move(_to_notify.account), settings = std::move(_to_notify.settings);
	int state = _to_notify.state;

	_to_notify.config.clear();
	_to_notify.account.clear();
	_to_notify.settings.clear();
	_to_notify.state = 0;

	if (config.size())
	{
		SendToAllClients(_rpc_client.BuildNotificationData("config", MakeConfigObject(&config)));
	}
	if (account.size())
	{
		SendToAllClients(_rpc_client.BuildNotificationData("account", MakeAccountObject(&account)));		
	}
	if (settings.size())
	{
		SendToAllClients(_rpc_client.BuildNotificationData("settings", MakeSettingsObject(&settings)));
	}
	if (state != 0)
	{
		SendToAllClients(_rpc_client.BuildNotificationData("state", MakeStateObject(state)));
	}

	if (config.count("locations"))
		PingServers();

	if (state & (STATE | CONNECT) || settings.count("firewall") || settings.count("allowLAN"))
		ApplyFirewallSettings();
}


JsonObject CypherDaemon::MakeConfigObject(const std::unordered_set<std::string>* keys)
{
	JsonObject config = FilterJsonObject(g_config.map(), keys);
	if (!keys || keys->count("certificateAuthorities")) config["certificateAuthorities"] = g_config.certificateAuthorities();
	return config;
}

JsonObject CypherDaemon::MakeAccountObject(const std::unordered_set<std::string>* keys)
{
	return FilterJsonObject(g_account.map(), keys);
}

JsonObject CypherDaemon::MakeSettingsObject(const std::unordered_set<std::string>* keys)
{
	return FilterJsonObject(g_settings.map(), keys);
}

JsonObject CypherDaemon::MakeStateObject(int flags)
{
	JsonObject state;
	if (flags & CONNECT)        state["connect"]        = _shouldConnect;
	if (flags & STATE)          state["state"]          = GetStateString(_state);
	if (flags & NEEDSRECONNECT) state["needsReconnect"] = _needsReconnect;
	if (flags & IPADDRESS)      state["localIP"]        = (_state == CONNECTED) ? JsonValue(_localIP) : nullptr;
	if (flags & IPADDRESS)      state["remoteIP"]       = (_state == CONNECTED) ? JsonValue(_remoteIP) : nullptr;
	if (flags & BYTECOUNT)      state["bytesReceived"]  = (_state == CONNECTED) ? JsonValue(_bytesReceived) : nullptr;
	if (flags & BYTECOUNT)      state["bytesSent"]      = (_state == CONNECTED) ? JsonValue(_bytesSent) : nullptr;
	if (flags & PING_STATS)     state["pingStats"]      = _ping_stats;
	return state;
}


JsonObject CypherDaemon::RPC_get(const std::string& type)
{
	if (type == "state")
		return MakeStateObject();
	if (type == "settings")
		return g_settings.map();
	if (type == "account")
		return MakeAccountObject();
	if (type == "config")
		return MakeConfigObject();
	throw jsonrpc::InvalidParametersFault();
}


void CypherDaemon::RPC_setAccount(const JsonObject& account)
{
	g_account.Reset();
	for (auto& p : account)
	{
		try
		{
			g_account.Set(p.first, p.second);
		}
		catch (std::exception& e)
		{
			LOG(WARNING) << "Received bad account item from client: " << p.first << " = " << p.second;
		}
	}
	// Immediately apply changes instead of waiting for the next slice.
	NotifyChanges();
}

void CypherDaemon::RPC_applyConfig(const JsonObject& config)
{
	for (auto& p : config)
	{
		if (g_config[p.first] != p.second)
		{
			try
			{
				g_config.Set(p.first, p.second);
			}
			catch (std::exception& e)
			{
				LOG(WARNING) << "Received bad config item from client: " << p.first << " = " << p.second;
			}
		}
	}
	// Immediately apply changes instead of waiting for the next slice.
	NotifyChanges();
}

void CypherDaemon::RPC_applySettings(const JsonObject& settings)
{
	// Look up all keys first to throw if there are any invalid settings
	// before we actually make any changes
	//for (auto& p : settings) g_settings.map().at(p.first);

	bool neededReconnect = _needsReconnect;

	for (auto& p : settings)
	{
		if (g_settings[p.first] != p.second)
		{
			try
			{
				g_settings.Set(p.first, p.second);
			}
			catch (std::exception& e)
			{
				LOG(WARNING) << "Received bad setting from client: " << p.first << " = " << p.second;
			}
		}
	}

	if ((_state == CONNECTING || _state == CONNECTED) && !_process->IsSameServer(g_settings.map()))
	{
		_needsReconnect = true;
		OnStateChanged(NEEDSRECONNECT);
	}

	// Immediately apply changes instead of waiting for the next slice.
	NotifyChanges();
}


void CypherDaemon::WriteOpenVPNProfile(std::ostream& out, const JsonObject& server, OpenVPNProcess* process)
{
	using namespace std;

	// Parse out protocol and remote port from remotePort setting
	std::string protocol;
	std::string remotePort;
	{
		auto both = SplitToVector(g_settings.remotePort(), ':', 1);
		protocol = std::move(both[0]);
		if (protocol == "tcp")
			protocol = "tcp-client";
		else if (protocol != "udp")
		{
			LOG(WARNING) << "Unrecognized 'remotePort' protocol '" << protocol << "', defaulting to 'udp'";
			protocol = "udp";
		}
		if (both.size() == 2)
		{
			remotePort = std::move(both[1]);
			if (remotePort == "auto")
				remotePort = "7133";
			else
			{
				try
				{
					size_t pos;
					unsigned long port = std::stoul(remotePort, &pos);
					if (pos != remotePort.size())
					{
						LOG(WARNING) << "Failed to fully parse 'remotePort' port '" << remotePort << "', using '" << port << "'";
						remotePort = std::to_string(port);
					}
				}
				catch (...)
				{
					LOG(WARNING) << "Unrecognized 'remotePort' port '" << remotePort << "', defaulting to '7133'";
					remotePort = "7133";
				}
			}
		}
	}

	// Basic settings
	out << "client" << endl;
	out << "dev tun" << endl;
	out << "proto " << protocol << endl;

	// MTU settings
	const int mtu = g_settings.mtu();
	out << "tun-mtu " << mtu << endl;
	//out << "fragment" << (mtu - 100) << endl;
	out << "mssfix " << (mtu - 220) << endl;

	// Default connection settings
	out << "ping 10" << endl;
	out << "ping-exit 60" << endl;
	out << "resolv-retry 0" << endl;
	out << "persist-remote-ip" << endl;
	//out << "persist-tun" << endl;
	out << "route-delay 0" << endl;

	// Default security settings
	out << "tls-cipher TLS-DHE-RSA-WITH-AES-256-GCM-SHA384:TLS-DHE-RSA-WITH-AES-256-CBC-SHA256:TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256" << endl;
	out << "auth SHA256" << endl;
	out << "tls-version-min 1.2" << endl;
	out << "remote-cert-eku \"TLS Web Server Authentication\"" << endl;
	out << "verify-x509-name " << server.at("ovHostname").AsString() << " name" << endl;
	out << "auth-user-pass" << endl;
	out << "ncp-disable" << endl;

	// Default route setting
	if (g_settings.routeDefault())
	{
		out << "redirect-gateway def1 bypass-dhcp";
		if (!g_settings.overrideDNS())
			out << " bypass-dns";
		// TODO: one day, redirect ipv6 traffic as well
		// out << " ipv6";
		out << endl;
	}

	// Local port setting
	if (g_settings.localPort() == 0)
		out << "nobind" << endl;
	else
		out << "lport " << g_settings.localPort() << endl;

	// Overridden encryption settings
	const auto& encryption = g_settings.encryption();
	if (encryption == "stealth")
	{
		out << "cipher AES-128-GCM" << endl;
		out << "scramble obfuscate cypherpunk-xor-key" << endl;
	}
	else if (encryption == "strong")
	{
		out << "cipher AES-256-GCM" << endl;
	}
	else if (encryption == "none")
	{
		out << "cipher none" << endl;
	}
	else // encryption == "default"
	{
		out << "cipher AES-128-GCM" << endl;
	}

	if (protocol == "udp")
	{
		// Always try to send the server a courtesy exit notification in UDP mode
		out << "explicit-exit-notify" << endl;
	}

	// Wait 15s before giving up on a connection and trying the next one
	out << "server-poll-timeout 10s" << endl;


	// Connection remotes; must come after generic connection settings
	std::string ipKey = "ov" + encryption;
	ipKey[2] = std::toupper(ipKey[2]);
	const auto& serverIPs = server.at(ipKey).AsArray();
	for (const auto& ip : serverIPs)
	{
		out << "<connection>" << endl;
		out << "  remote " << ip.AsString() << ' ' << remotePort << endl;
		out << "</connection>" << endl;
	}
	process->connection_retries_left = serverIPs.size() - 1;

	// Extra routes; currently only used by the "exempt Apple services" setting
#if OS_OSX
	if (g_settings.exemptApple())
	{
		out << "route 17.0.0.0 255.0.0.0 net_gateway" << endl;
	}
#endif

	// Output DNS settings; these are done via dhcp-option switches,
	// and depend on the "Use Cypherpunk DNS" and related settings.

	// Tell OpenVPN to always ignore the pushed DNS (10.10.10.10),
	// which is simply there as a sensible default for dumb clients.
	out << "pull-filter ignore \"dhcp-option DNS 10.10.10.10\"" << endl;
	out << "pull-filter ignore \"route 10.10.10.10 255.255.255.255\"" << endl;

	if (g_settings.overrideDNS())
	{
		int dns_index = 10
			+ (g_settings.blockAds() ? 1 : 0)
			+ (g_settings.blockMalware() ? 2 : 0)
			+ (g_settings.optimizeDNS() ? 4 : 0);
		std::string dns_string = std::to_string(dns_index);
		out << "dhcp-option DNS 10.10.10." << dns_string << endl;
		out << "route 10.10.10." << dns_string << endl;
#if OS_LINUX
		// On Linux, simulate secondary/tertiary DNS servers in order to push any prior DNS out of the list
		out << "dhcp-option DNS 10.10.11." << dns_string << endl;
		out << "dhcp-option DNS 10.10.12." << dns_string << endl;
		out << "route 10.10.11." << dns_string << endl;
		out << "route 10.10.12." << dns_string << endl;
#elif OS_WIN
		// On Windows, add some additional convenience/robustness switches
		out << "register-dns" << endl;
		if (g_settings.routeDefault())
			out << "block-outside-dns" << endl;
#endif
	}

	// Include hardcoded certificate authority
	out << "<ca>" << endl;
	for (auto& ca : g_config.certificateAuthorities())
		for (auto& line : ca.AsArray())
			out << line.AsString() << endl;
	out << "</ca>" << endl;
}

bool CypherDaemon::RPC_connect()
{
	// FIXME: Shouldn't simply read raw profile parameters from the params
	const JsonObject& settings = g_settings.map();

	// Access the region early, should trigger an exception if it doesn't exist (before we've done any state changes)
	g_settings.currentLocation();

	{
		static const int MAX_RECENT_ITEMS = 3;
		JsonArray recent = g_settings.recent();
		recent.erase(std::remove(recent.begin(), recent.end(), g_settings.location()), recent.end());
		recent.insert(recent.begin(), g_settings.location());
		if (recent.size() > MAX_RECENT_ITEMS)
			recent.resize(MAX_RECENT_ITEMS);
		g_settings.recent(recent);
	}

	switch (_state)
	{
		case CONNECTED:
		case CONNECTING:
			// Reconnect only if settings have changed
			if (!_needsReconnect && _process->IsSameServer(settings))
			{
				LOG(INFO) << "No need to reconnect; new server is the same as old";
				break;
			}
			// fallthrough
		case DISCONNECTING:
			// Reconnect
			_process->stale = true;
			_bytesReceived = _bytesSent = 0;
			_state = SWITCHING;
			OnStateChanged(STATE | BYTECOUNT);
			_process->SendManagementCommand("signal SIGTERM");
			break;

		case SWITCHING:
			// Already switching; make sure connection is marked as stale
			_process->stale = true;
			break;

		case DISCONNECTED:
			// Connect normally.
			_bytesReceived = _bytesSent = 0;
			_state = CONNECTING;
			OnStateChanged(STATE | BYTECOUNT);
			_io.post([this](){ DoConnect(); });
			break;

		default:
			// Can't connect under other circumstances.
			return false;
	}
	if (!_shouldConnect)
	{
		_shouldConnect = true;
		OnStateChanged(CONNECT);
	}
	NotifyChanges();
	return true;
}

void CypherDaemon::DoConnect()
{
	static int index = 0;
	//index++; // FIXME: Just make GetAvailablePort etc. work properly instead

	_needsReconnect = false;
	OnStateChanged(NEEDSRECONNECT);

	std::shared_ptr<OpenVPNProcess> vpn(CreateOpenVPNProcess(_ws_server.get_io_service()));
	vpn->SetSettings(g_settings.map());

	int port = vpn->StartManagementInterface();

	_process = vpn;

	std::vector<std::string> args;

	args.push_back("--management");
	args.push_back("127.0.0.1");
	args.push_back(std::to_string(port));
	args.push_back("--management-hold");
	args.push_back("--management-client");
	args.push_back("--management-query-passwords");

	args.push_back("--verb");
	args.push_back("4");

#if OS_WIN
	args.push_back("--dev-node");
	args.push_back(GetAvailableAdapter(index));
#elif OS_OSX
	args.push_back("--dev-node");
	args.push_back("utun");
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
#elif OS_LINUX
	args.push_back("--script-security");
	args.push_back("2");

	args.push_back("--up");
	args.push_back(GetPath(ScriptsDir, "updown.sh"));
	args.push_back("--down");
	args.push_back(GetPath(ScriptsDir, "updown.sh"));
#elif OS_WIN
	args.push_back("--script-security");
	args.push_back("1");
#else
	args.push_back("--script-security");
	args.push_back("1");
#endif

	args.push_back("--config");

	char profile_filename_tmp[32];
	snprintf(profile_filename_tmp, sizeof(profile_filename_tmp), "profile%d.ovpn", index);
	std::string profile_filename = GetPath(ProfileDir, EnsureExists, profile_filename_tmp);
	{
		std::ofstream f(profile_filename.c_str());
		WriteOpenVPNProfile(f, g_settings.currentLocation(), vpn.get());
		f.close();
	}
	args.push_back(profile_filename);

	vpn->OnManagementResponse("HOLD", [=](const std::string& line) {
		vpn->SendManagementCommand("hold release");
	});
	vpn->OnManagementResponse("PASSWORD", [=](const std::string& line) {
		LOG(INFO) << line;
		size_t q1 = line.find('\'');
		size_t q2 = line.find('\'', q1 + 1);
		auto id = line.substr(q1 + 1, q2 - q1 - 1);
		vpn->SendManagementCommand(
			"username \"" + id + "\" \"" + vpn->_username + "\"\n"
			"password \"" + id + "\" \"" + vpn->_password + "\"");
	});
	vpn->OnManagementResponse("STATE", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			if (params.size() >= 2)
			{
				const auto& s = params[1];
				if (_state == SWITCHING)
				{
					if (s == "CONNECTED" || s == "RECONNECTING")
					{
						// Default handling below
					}
					else if (s == "EXITING")
					{
						if (_process->stale)
						{
							_process.reset();
							DoConnect();
							return;
						}
						// Default handling below
					}
					else
						return;
				}
				if (s == "CONNECTED")
				{
					_process->connection_retries_left = 1;
					if (_state == DISCONNECTING)
					{
						_process->SendManagementCommand("signal SIGTERM");
						return;
					}
					if (params.size() >= 5)
					{
						_localIP = params.at(3);
						_remoteIP = params.at(4);
					}
					_state = CONNECTED;
					OnStateChanged(STATE | IPADDRESS);
				}
				else if (s == "RECONNECTING")
				{
					if (_state == CONNECTED)
					{
						_state = CONNECTING;
						OnStateChanged(STATE);
					}
					else if (_state == CONNECTING && (_process->connection_retries_left == 0 || --_process->connection_retries_left == 0))
					{
						_process->SendManagementCommand("signal SIGTERM");
						// TODO: Send an error message to the client
						// TODO: Implement a way to send error messages to the client
					}
				}
				else if (s == "EXITING")
				{
					_process.reset();
					_state = DISCONNECTED;
					_needsReconnect = false;
					OnStateChanged(STATE | NEEDSRECONNECT);
				}
			}
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << e;
			_state = DISCONNECTED;
			_needsReconnect = false;
			if (_process)
			{
				_process->Kill();
				_process.reset();
			}
		}
	});
	vpn->OnManagementResponse("BYTECOUNT", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			_bytesReceived = std::stoll(params[0]);
			_bytesSent = std::stoll(params[1]);
			OnStateChanged(BYTECOUNT);
		}
		catch (const std::exception& e) { LOG(WARNING) << e; }
	});
	vpn->OnManagementResponse("LOG", [=](const std::string& line) {
		auto params = SplitToVector(line, ',', 2);
		LogLevel level;
		if (params[1] == "F") level = LogLevel::CRITICAL;
		else if (params[1] == "N") level = LogLevel::ERROR;
		else if (params[1] == "W") level = LogLevel::WARNING;
		else if (params[1] == "I") level = LogLevel::INFO;
		else /*if (params[1] == "D")*/ level = LogLevel::VERBOSE;
		LOG_EX(level, true, Location("openvpn")) << params[2];
	});

	vpn->Run(args);
	vpn->AsyncWait([this, vpn](const asio::error_code& error) {
		OnOpenVPNProcessExited(vpn.get());
	});

	vpn->SendManagementCommand("state on\nbytecount 5\nhold release");
}

void CypherDaemon::RPC_disconnect()
{
	if (_shouldConnect)
	{
		_shouldConnect = false;
		OnStateChanged(CONNECT);
	}
	if (_state == CONNECTING || _state == CONNECTED || _state == SWITCHING)
	{
		_state = DISCONNECTING;
		OnStateChanged(STATE);
		_process->SendManagementCommand("signal SIGTERM");
	}
	NotifyChanges();
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

void CypherDaemon::PingServers()
{
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<double> cutoff = (now - std::chrono::minutes(60)).time_since_epoch();
	auto pinger = std::make_shared<ServerPingerThinger>(_io);
	for (const auto& p : g_config.locations())
	{
		try
		{
			try
			{
				if (_ping_stats.at(p.first).AsStruct().at("lastChecked").AsDouble() >= cutoff.count())
					continue;
			}
			catch (...) {}
			pinger->Add(p.first, p.second.AsStruct().at("ovDefault").AsArray().at(0).AsString());
		}
		catch (...) {}
	}
	pinger->Start(5, [this](std::vector<ServerPingerThinger::Result> results) {
		std::chrono::duration<double> now = std::chrono::steady_clock::now().time_since_epoch();
		for (auto& r : results)
		{
			LOG(INFO) << "Ping " << r.id << ": avg=" << (r.average * 1000) << " min=" << (r.minimum * 1000) << " max=" << (r.maximum * 1000) << " replies=" << r.replies << " timeouts=" << r.timeouts;
			JsonObject obj;
			if (r.replies > 0 || r.timeouts > 0)
				obj.emplace("lastChecked", now.count());
			obj.emplace("average", r.average);
			obj.emplace("minimum", r.minimum);
			obj.emplace("maximum", r.maximum);
			obj.emplace("replies", (signed)r.replies);
			obj.emplace("timeouts", (signed)r.timeouts);
			_ping_stats[r.id] = std::move(obj);
		}
		OnStateChanged(PING_STATS);
	});
}
