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

extern class CypherDaemon* g_daemon;

class OpenVPNProcess;
typedef jsonrpc::Server JsonRPCServer;
typedef jsonrpc::Client JsonRPCClient;
typedef jsonrpc::Dispatcher JsonRPCDispatcher;


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

public:
	// The main entrypoint of the daemon; should block for the duration of the daemon.
	virtual int Run();

	// Requests that the daemon begin shutting down. Can be called either from within
	// the daemon or from another thread.
	virtual void RequestShutdown();

	void OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line);
	void OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line);
	void OnOpenVPNProcessExited(OpenVPNProcess* process);
	void OnSettingsChanged(const std::vector<std::string>& names);

protected:
	typedef websocketpp::connection_hdl Connection;
	typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

	enum StateChangedFlags { STATE = 1, NEEDSRECONNECT = 2, IPADDRESS = 4, BYTECOUNT = 8 };

	void SendToClient(Connection con, const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendErrorToAllClients(const std::string& name, const std::string& description);
	void OnFirstClientConnected();
	void OnClientConnected(Connection c);
	void OnClientDisconnected(Connection c);
	void OnLastClientDisconnected();
	void OnReceiveMessage(Connection con, WebSocketServer::message_ptr msg);
	void OnStateChanged(unsigned int state_changed_flags);

	JsonObject MakeStateObject();
	JsonObject MakeConfigObject();
	JsonObject MakeAccountObject();

	// Get one of the datasets (state, settings or config).
	JsonObject RPC_get(const std::string& type);
	// Apply one or more settings.
	void RPC_applySettings(const JsonObject& settings);
	// Set the user account info. FIXME: should probably have the daemon be in charge of this instead, or at least validate the information independently from the client.
	void RPC_setAccount(const JsonObject& account);
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
	State _state;
	std::string _localIP, _remoteIP;
	int64_t _bytesReceived, _bytesSent;
	bool _needsReconnect;
	//std::map<std::string, ServerInfo> _servers;

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

