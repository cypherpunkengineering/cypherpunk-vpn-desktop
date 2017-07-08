#pragma once

#if OS_LINUX
#include <string.h>
#endif

#include "config.h"
#include "connection.h"
#include "openvpn.h"
#include "settings.h"
#include "websocketpp.h"

#include <asio.hpp>

#include <jsonrpc-lean/server.h>
#include <jsonrpc-lean/client.h>

#include <map>
#include <memory>
#include <set>
#include <thread>
#include <unordered_set>

extern class CypherDaemon* g_daemon;

class OpenVPNProcess;
typedef jsonrpc::Server JsonRPCServer;
typedef jsonrpc::Client JsonRPCClient;
typedef jsonrpc::Dispatcher JsonRPCDispatcher;

// Implemented by platform
extern unsigned short GetPingIdentifier();


/*struct ServerInfo
{
	std::string id;
	std::string name;
	std::string country;
	double latitude, longitude;
	std::map<std::string, std::string> ips;
};*/

class CypherDaemon : public ConnectionListener
{
	static const int DEFAULT_RPC_PORT = 9337;
	static const int DEFAULT_OPENVPN_PORT_BASE = 9338;

public:
	CypherDaemon();

	enum State
	{
		STARTING,
		INITIALIZED,
		// Actual state fetched from _connection
		EXITING,
		EXITED,
	};

	struct PingResult
	{
		std::string id;
		double average, minimum, maximum;
		size_t replies, timeouts;
	};
	typedef void PingCallback(std::vector<PingResult> results);

public:
	// The main entrypoint of the daemon; should block for the duration of the daemon.
	virtual int Run();

	// Requests that the daemon begin shutting down. Can be called either from within
	// the daemon or from another thread.
	virtual void RequestShutdown();

	virtual void OnBeforeRun() {}
	virtual void OnAfterRun() {}

	virtual void OnOpenVPNCallback(OpenVPNProcess* process, std::string line) override {}
	virtual void OnConfigChanged(const char* name);
	virtual void OnAccountChanged(const char* name);
	virtual void OnSettingsChanged(const char* name);

protected:
	typedef websocketpp::connection_hdl ClientConnection;
	typedef std::set<ClientConnection, std::owner_less<ClientConnection>> ClientConnectionList;
	typedef std::map<ClientConnection, bool, std::owner_less<ClientConnection>> ClientConnectionMap;

	void SendToClient(ClientConnection con, const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendErrorToAllClients(std::string name, bool critical, std::string message);
	void OnFirstClientConnected();
	void OnClientConnected(ClientConnection c);
	void OnClientDisconnected(ClientConnection c);
	void OnLastClientDisconnected();
	void OnReceiveMessage(ClientConnection con, WebSocketServer::message_ptr msg);
	void OnStateChanged(unsigned int state_changed_flags);

	virtual void OnConnectionStateChanged(Connection* connection, Connection::State state) override;
	virtual void OnTrafficStatsUpdated(Connection* connection, uint64_t downloaded, uint64_t uploaded) override;
	virtual void OnConnectionAttempt(Connection* connection, int attempt) override;
	virtual void OnConnectionError(Connection* connection, Connection::ErrorCode error, bool critical, std::string message) override;

	void NotifyChanges();

	void PingServers();
	void ScheduleNextPingServers(std::chrono::steady_clock::time_point t);
	void WriteOpenVPNProfile(std::ostream& out, const JsonObject& server, OpenVPNProcess* process);

	JsonObject MakeConfigObject(const std::unordered_set<std::string>* keys = nullptr);
	JsonObject MakeAccountObject(const std::unordered_set<std::string>* keys = nullptr);
	JsonObject MakeSettingsObject(const std::unordered_set<std::string>* keys = nullptr);
	JsonObject MakeStateObject(int flags = -1);

	// Get one of the datasets (state, settings or config).
	JsonObject RPC_get(const std::string& type);
	// Set the user account info. FIXME: should probably have the daemon be in charge of this instead, or at least validate the information independently from the client.
	void RPC_setAccount(const JsonObject& account);
	// Apply config settings (settings that are not supposed to be touched directly by the user)
	void RPC_applyConfig(const JsonObject& config);
	// Apply one or more settings.
	void RPC_applySettings(const JsonObject& settings);
	// Reset settings to default values
	void RPC_resetSettings(bool deleteAllValues = true);
	// Connect to the currently configured server. Returns false if already
	// connected and no changes are required.
	bool RPC_connect();
	// Disconnect from the current server (or cancel a connection attempt).
	void RPC_disconnect();

	//void DoConnect();
	void DoShutdown();

	asio::io_service _io;
	WebSocketServer _ws_server;
	ClientConnectionList _connections;
	jsonrpc::JsonFormatHandler _json_handler;
	JsonRPCDispatcher _dispatcher;
	JsonRPCClient _rpc_client;
	std::shared_ptr<Connection> _connection;
	
	// Bitfield to internally signal which state-related fields have changed
	enum StateChangedFlags
	{
		CONNECT = 1,         // _shouldConnect
		STATE = 2,           // _state
		NEEDSRECONNECT = 4,  // _needsReconnect
		BYTECOUNT = 8,       // _bytesSent, _bytesReceived
		PING_STATS = 16,     // _ping_stats
	};

	// Client has clicked the connect button and wants to be online
	bool _shouldConnect;
	// Current daemon state
	State _state;
	// Some setting has changed which requires a reconnect (i.e. call RPC_connect again)
	bool _needsReconnect;
	// Traffic stats
	int64_t _bytesReceived, _bytesSent;
	// Ping statistics for 
	JsonObject _ping_stats;
	struct {
		std::unordered_set<std::string> config, account, settings;
		int state; // StateChangedFlags
	} _to_notify;
	bool _notify_scheduled;
	//std::map<std::string, ServerInfo> _servers;

	asio::basic_waitable_timer<std::chrono::steady_clock> _ping_timer;
	std::chrono::steady_clock::time_point _last_ping_round;
	bool _next_ping_scheduled;


public:
	// Ask the system for an available TCP port (for listening), preferably >= 'hint'.
	virtual int GetAvailablePort(int hint);
	// Get the identifier (for --dev) for an available TAP adapter to use.
	virtual std::string GetAvailableAdapter(int index) = 0;
	// Apply the firewall/killswitch mode to the system.
	virtual void ApplyFirewallSettings() {}
};

