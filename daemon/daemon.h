#pragma once

#if OS_LINUX
#include <string.h>
#endif

#include "config.h"
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

class CypherDaemon
{
	static const int DEFAULT_RPC_PORT = 9337;
	static const int DEFAULT_OPENVPN_PORT_BASE = 9338;

public:
	CypherDaemon();

	enum State
	{
		STARTING,
		INITIALIZED,
		DISCONNECTED = INITIALIZED,
		CONNECTING,
		CONNECTED,
		SWITCHING,
		DISCONNECTING,
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

	virtual void OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line);
	virtual void OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line);
	virtual void OnOpenVPNCallback(OpenVPNProcess* process, std::string args);
	virtual void OnOpenVPNProcessExited(OpenVPNProcess* process);
	virtual void OnConfigChanged(const char* name);
	virtual void OnAccountChanged(const char* name);
	virtual void OnSettingsChanged(const char* name);

protected:
	typedef websocketpp::connection_hdl Connection;
	typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

	void SendToClient(Connection con, const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendErrorToAllClients(const std::string& name, const std::string& description);
	void OnFirstClientConnected();
	void OnClientConnected(Connection c);
	void OnClientDisconnected(Connection c);
	void OnLastClientDisconnected();
	void OnReceiveMessage(Connection con, WebSocketServer::message_ptr msg);
	void OnStateChanged(unsigned int state_changed_flags);

	void NotifyChanges();

	void PingServers();
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
	// Connect to the currently configured server. Returns false if already
	// connected and no changes are required.
	bool RPC_connect();
	// Disconnect from the current server (or cancel a connection attempt).
	void RPC_disconnect();

	void DoConnect();

	asio::io_service _io;
	WebSocketServer _ws_server;
	ConnectionList _connections;
	jsonrpc::JsonFormatHandler _json_handler;
	JsonRPCDispatcher _dispatcher;
	JsonRPCClient _rpc_client;
	std::shared_ptr<OpenVPNProcess> _process;
	
	// Bitfield to internally signal which state-related fields have changed
	enum StateChangedFlags
	{
		CONNECT = 1,         // _shouldConnect
		STATE = 2,           // _state
		NEEDSRECONNECT = 4,  // _needsReconnect
		IPADDRESS = 8,       // _localIP, _remoteIP
		BYTECOUNT = 16,      // _bytesSent, _bytesReceived
		PING_STATS = 32,     // _ping_stats
	};

	// Client has clicked the connect button and wants to be online
	bool _shouldConnect;
	// Actual current daemon/connection state
	State _state;
	// Some setting has changed which requires a reconnect (i.e. call RPC_connect again)
	bool _needsReconnect;
	// IP addresses for the current connection (invalid when _state != CONNECTED)
	std::string _localIP, _remoteIP, _publicIP;
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

	// Number of reconnection to tolerate before we should treat as disconnected. Initally set to the number of <connection> entries
	size_t _connection_retries_left;
	bool _was_ever_connected;


protected:
	// Create a platform-specific handler around an OpenVPN process.
	// Note: the process isn't actually started until 'Run' is called on it.
	virtual OpenVPNProcess* CreateOpenVPNProcess(asio::io_service& io) = 0;
	// Ask the system for an available TCP port (for listening), preferably >= 'hint'.
	virtual int GetAvailablePort(int hint);
	// Get the identifier (for --dev) for an available TAP adapter to use.
	virtual std::string GetAvailableAdapter(int index) = 0;
	// Apply the firewall/killswitch mode to the system.
	virtual void ApplyFirewallSettings() {}
};

