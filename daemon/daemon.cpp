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


class AutoDeleteFile
{
	FILE* _file;
	std::string _name;
public:
	AutoDeleteFile(std::string path, const char* mode) : _file(NULL), _name(std::move(path))
	{
		_file = daemon_fopen(_name.c_str(), mode);
	}
	~AutoDeleteFile()
	{
		if (_file)
		{
			daemon_unlink(_name.c_str());
			daemon_fclose(_file);
		}
	}
	operator FILE*() const { return _file; }
	bool operator !() const { return !_file; }
};


CypherDaemon::CypherDaemon()
	: _rpc_client(_json_handler)
	, _state(STARTING)
	, _shouldConnect(false)
	, _needsReconnect(false)
	, _notify_scheduled(false)
	, _ping_timer(_io)
	, _next_ping_scheduled(false)
{

}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon version v" VERSION " built on " __TIMESTAMP__;

	_ws_server.set_message_handler(std::bind(&CypherDaemon::OnReceiveMessage, this, _1, _2));
	_ws_server.set_open_handler([this](ClientConnection c) {
		bool first = _connections.empty();
		_connections.insert(c);
		if (first) OnFirstClientConnected();
		OnClientConnected(c);
	});
	_ws_server.set_close_handler([this](ClientConnection c) {
		try
		{
			_connections.erase(c);
		}
		catch (...) {}
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

	{
		auto& d = _dispatcher;
		d.AddMethod("get", &CypherDaemon::RPC_get, *this);
		d.AddMethod("connect", &CypherDaemon::RPC_connect, *this);
		d.AddMethod("disconnect", &CypherDaemon::RPC_disconnect, *this);
		d.AddMethod("setAccount", &CypherDaemon::RPC_setAccount, *this);
		d.AddMethod("applyConfig", &CypherDaemon::RPC_applyConfig, *this);
		d.AddMethod("applySettings", &CypherDaemon::RPC_applySettings, *this);
		d.AddMethod("resetSettings", &CypherDaemon::RPC_resetSettings, *this);
		d.AddMethod("ping", [](){});
	}

	g_config.ReadFromDisk();
	g_account.ReadFromDisk();
	g_settings.ReadFromDisk();

	g_config.SetOnChangedHandler(THIS_CALLBACK(OnConfigChanged));
	g_account.SetOnChangedHandler(THIS_CALLBACK(OnAccountChanged));
	g_settings.SetOnChangedHandler(THIS_CALLBACK(OnSettingsChanged));

	_connection = std::make_shared<Connection>(_io, this);

	int port;
	try
	{
		_ws_server.init_asio(&_io);
		_ws_server.set_reuse_addr(true);
		try
		{
			_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 9337));
		}
		catch (...)
		{
			// Debug builds can't run on anything but 9337; fail.
			if (!IsInstalled()) throw;

			// Failed to open default port 9337, use any port instead
			_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0));
		}
		_ws_server.start_accept();

		asio::error_code ec;
		auto endpoint = _ws_server.get_local_endpoint(ec);
		asio::detail::throw_error(ec, "get_local_endpoint");
		port = endpoint.port();
	}
	catch (const std::exception& e)
	{
		LOG(ERROR) << "Unable to open a listening port: " << e;
		return 1;
	}

	AutoDeleteFile port_file(GetFile(DaemonPortFile), "w");
	if (!port_file || fprintf(port_file, "%d", port) < 0)
	{
		LOG(ERROR) << "Unable to write port file";
		return 1;
	}
	fflush(port_file);
	LOG(INFO) << "Listening on port " << port;

	_state = INITIALIZED;
	int result = 0;

	OnBeforeRun();

	try
	{
		_ws_server.run(); // internally calls _io.run()
	}
	catch (const std::exception& e)
	{
		LOG(ERROR) << "Exception thrown in main loop: " << e;
		result = 1;
	}

	OnAfterRun();

	_state = EXITED;

	return result;
}

void CypherDaemon::RequestShutdown()
{
	if (_state < EXITING)
	{
		_state = EXITING;
		OnStateChanged(STATE);
	}
	_io.post([this]() { DoShutdown(); });
}

void CypherDaemon::DoShutdown()
{
	// 1. Shut down any existing OpenVPN process
	if (_shouldConnect)
	{
		_shouldConnect = false;
		OnStateChanged(CONNECT);
	}
	if (_connection && _connection->GetState() != Connection::DISCONNECTED)
	{
		LOG(INFO) << "Killing OpenVPN";
		_connection->Disconnect();
		return; // OnConnectionStateChanged will call DoShutdown again
	}
	// Give the killswitch one additional chance to notice and disengage,
	// as it may not have received the CONNECT state change notification.
	NotifyChanges();

	// 2. Disconnect all clients
	if (!_connections.empty())
	{
		LOG(INFO) << "Disconnecting all clients";
		for (auto& c : _connections)
		{
			_ws_server.pause_reading(c);
			_ws_server.close(c, websocketpp::close::status::going_away, "");
		}
		return; // OnLastClientDisconnected will call DoShutdown again
	}

	// 3. Finally, kill the message loop
	LOG(INFO) << "Stopping message loop";
	if (!_ws_server.stopped())
		_ws_server.stop();
}

void CypherDaemon::OnConnectionStateChanged(Connection* connection, Connection::State state)
{
	if (_state == EXITING && state == Connection::DISCONNECTED)
	{
		_io.post([this]() { DoShutdown(); });
	}
	else if (_state == INITIALIZED)
	{
		OnStateChanged(STATE);
		if (state == Connection::DISCONNECTED && _shouldConnect)
		{
			_shouldConnect = false;
			OnStateChanged(CONNECT);
		}
	}
}

void CypherDaemon::OnTrafficStatsUpdated(Connection* connection, uint64_t downloaded, uint64_t uploaded)
{
	_bytesReceived = downloaded;
	_bytesSent = uploaded;
	OnStateChanged(BYTECOUNT);
}

void CypherDaemon::OnConnectionAttempt(Connection* connection, int attempt)
{
	if (_needsReconnect)
	{
		_needsReconnect = false;
		OnStateChanged(NEEDSRECONNECT);
	}
}

void CypherDaemon::SendToClient(ClientConnection connection, const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	LOG(DEBUG) << "Sending RPC message: " << std::string(data->GetData(), data->GetSize());
	_ws_server.send(connection, data->GetData(), data->GetSize(), websocketpp::frame::opcode::TEXT);
}

void CypherDaemon::SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	for (auto& c : _connections)
		SendToClient(c, data);
}

void CypherDaemon::SendErrorToAllClients(std::string name, bool critical, std::string message)
{
	JsonObject desc;
	desc["name"] = std::move(name);
	desc["critical"] = critical;
	desc["message"] = std::move(message);
	SendToAllClients(_rpc_client.BuildNotificationData("error", std::move(desc)));
}

void CypherDaemon::OnFirstClientConnected()
{
	// If the kill-switch is set to always on, trigger it when the first client connects
	if (g_settings.firewall() == "on")
		ApplyFirewallSettings();
	PingServers();
}

void CypherDaemon::OnClientConnected(ClientConnection c)
{
	JsonObject data;
	data["version"] = JsonString(VERSION);
	data["config"] = MakeConfigObject();
	data["account"] = MakeAccountObject();
	data["settings"] = MakeSettingsObject();
	data["state"] = MakeStateObject();
	SendToClient(c, _rpc_client.BuildNotificationData("data", std::move(data)));
}

void CypherDaemon::OnClientDisconnected(ClientConnection c)
{

}

void CypherDaemon::OnLastClientDisconnected()
{
	if (_shouldConnect)
	{
		_shouldConnect = false;
		OnStateChanged(CONNECT);
	}
	if (_state == INITIALIZED)
		_connection->Disconnect();

	_ping_timer.cancel();
	_next_ping_scheduled = false;

	// If the kill-switch is set to always on, trigger it here so it can be disabled when
	// the last client disconnects (this also happens above inside OnStateChanged(STATE))
	if (g_settings.firewall() == "on")
		ApplyFirewallSettings();

	if (_state == EXITING)
		_io.post([this]() { DoShutdown(); });
}

void CypherDaemon::OnReceiveMessage(ClientConnection connection, WebSocketServer::message_ptr msg)
{
	LOG(DEBUG) << "Received RPC message: " << msg->get_payload();
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
				SendToClient(connection, _rpc_client.BuildNotificationData("data", response.GetResult()));
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
	CASE(INITIALIZED)
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

	JsonObject data;

	if (config.size())
	{
		data["config"] = MakeConfigObject(&config);
		g_config.WriteToDisk();
	}
	if (account.size())
	{
		data["account"] = MakeAccountObject(&account);
		g_account.WriteToDisk();
	}
	if (settings.size())
	{
		data["settings"] = MakeSettingsObject(&settings);
		g_settings.WriteToDisk();
	}
	if (state != 0)
	{
		data["state"] = MakeStateObject(state);
	}

	SendToAllClients(_rpc_client.BuildNotificationData("data", std::move(data)));

	if (config.count("locations"))
		PingServers();

	if (state & (STATE | CONNECT) || settings.count("firewall") || settings.count("allowLAN") || settings.count("overrideDNS"))
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
	if (flags & CONNECT)
	{
		state["connect"] = _shouldConnect;
	}
	if (flags & STATE)
	{
		state["state"] = (_state == INITIALIZED) ? _connection->GetStateString() : GetStateString(_state);
	}
	if (flags & NEEDSRECONNECT)
	{
		state["needsReconnect"] = _needsReconnect;
	}
	if (flags & BYTECOUNT)
	{
		if (_state == INITIALIZED && _connection->GetState() == Connection::CONNECTED)
		{
			state["bytesReceived"]  = JsonValue(_bytesReceived);
			state["bytesSent"]      = JsonValue(_bytesSent);
		}
		else
		{
			state["bytesReceived"]  = nullptr;
			state["bytesSent"]      = nullptr;			
		}
	}
	if (flags & PING_STATS)
	{
		state["pingStats"] = _ping_stats;
	}
	return state;
}


JsonObject CypherDaemon::RPC_get(const std::string& type)
{
	JsonObject data;
	if (type == "state" || type == "all")
		data["state"] = MakeStateObject();
	if (type == "settings" || type == "all")
		data["settings"] = MakeSettingsObject();
	if (type == "account" || type == "all")
		data["account"] = MakeAccountObject();
	if (type == "config" || type == "all")
		data["config"] = MakeConfigObject();
	return data;
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
		catch (const std::exception&)
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
			catch (const std::exception&)
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

	bool suppressReconnectWarning = false;

	for (auto& p : settings)
	{
		if (p.first == "suppressReconnectWarning" && p.second.IsTruthy())
		{
			suppressReconnectWarning = true;
			continue;
		}
		if (g_settings[p.first] != p.second)
		{
			try
			{
				g_settings.Set(p.first, p.second);
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << "Received bad setting from client: " << p.first << " = " << p.second;
			}
		}
	}

	if (!_needsReconnect && !suppressReconnectWarning && _state == INITIALIZED && _connection->NeedsReconnect())
	{
		_needsReconnect = true;
		OnStateChanged(NEEDSRECONNECT);
	}

	// Immediately apply changes instead of waiting for the next slice.
	NotifyChanges();
}

void CypherDaemon::RPC_resetSettings(bool deleteAllValues)
{
	g_settings.Reset(deleteAllValues);
	NotifyChanges();
}

bool CypherDaemon::RPC_connect()
{
	if (_state != INITIALIZED)
		return false;

	// Access required data early, should trigger an exception if it doesn't exist (before we've done any state changes)
	g_settings.currentLocation();
	g_account.privacy().at("username");
	g_account.privacy().at("password");

	// Update the lastConnected field of current location
	{
		std::chrono::duration<double> timestamp = std::chrono::steady_clock::now().time_since_epoch();
		g_settings.lastConnected()[g_settings.location()] = JsonValue(timestamp.count());
		// Secretly modified properties, need to explicitly notify
		g_settings.OnChanged("lastConnected");

		static const int MAX_RECENT_ITEMS = 3;
		JsonArray recent = g_settings.recent();
		recent.erase(std::remove(recent.begin(), recent.end(), g_settings.location()), recent.end());
		recent.insert(recent.begin(), g_settings.location());
		if (recent.size() > MAX_RECENT_ITEMS)
			recent.resize(MAX_RECENT_ITEMS);
		g_settings.recent(recent);
	}

	if (!_shouldConnect)
	{
		_shouldConnect = true;
		OnStateChanged(CONNECT);
	}

	bool result = _connection->Connect(_needsReconnect);

	if (_needsReconnect)
	{
		_needsReconnect = false;
		OnStateChanged(NEEDSRECONNECT);
	}

	NotifyChanges();

	return result;
}


void CypherDaemon::RPC_disconnect()
{
	if (_state != INITIALIZED)
		return;

	if (_shouldConnect)
	{
		_shouldConnect = false;
		OnStateChanged(CONNECT);
	}

	_connection->Disconnect();

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

void CypherDaemon::PingServers()
{
	static constexpr const auto PING_INTERVAL = std::chrono::minutes(5);
	auto now = std::chrono::steady_clock::now();
	_ping_timer.cancel();
	_last_ping_round = now;
	_next_ping_scheduled = false;
	auto stamp = now.time_since_epoch().count();
	std::chrono::duration<double> cutoff = (now - PING_INTERVAL).time_since_epoch();
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
	_ping_stats["updating"] = true;
	OnStateChanged(PING_STATS);
	pinger->Start(5, [this](std::vector<ServerPingerThinger::Result> results) {
		auto now = std::chrono::steady_clock::now();
		auto stamp = std::chrono::duration<double>(now.time_since_epoch()).count();
		for (auto& r : results)
		{
			LOG(INFO) << "Ping " << r.id << ": avg=" << (r.average * 1000) << " min=" << (r.minimum * 1000) << " max=" << (r.maximum * 1000) << " replies=" << r.replies << " timeouts=" << r.timeouts;
			JsonObject obj;
			if (r.replies > 0 || r.timeouts > 0)
				obj.emplace("lastChecked", stamp);
			obj.emplace("average", r.average);
			obj.emplace("minimum", r.minimum);
			obj.emplace("maximum", r.maximum);
			obj.emplace("replies", (signed)r.replies);
			obj.emplace("timeouts", (signed)r.timeouts);
			_ping_stats[r.id] = std::move(obj);
		}
		_ping_stats["updating"] = false;
		OnStateChanged(PING_STATS);
		ScheduleNextPingServers(now + PING_INTERVAL);
	});
}

void CypherDaemon::ScheduleNextPingServers(std::chrono::steady_clock::time_point t)
{
	if (!_next_ping_scheduled)
	{
		_next_ping_scheduled = true;
		_ping_timer.cancel();
		_ping_timer.expires_at(t);
		_ping_timer.async_wait([this](const asio::error_code& error) {
			if (!error) PingServers();
		});
	}
}
