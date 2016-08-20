#pragma once

#include "config.h"
#include "openvpn.h"
#include "settings.h"
#include "websocketpp.h"

#include <asio.hpp>

#include <jsonrpc-lean/server.h>
#include <jsonrpc-lean/client.h>

#include <set>
#include <thread>

extern class CypherDaemon* g_daemon;

class OpenVPNProcess;
typedef jsonrpc::Server JsonRPCServer;
typedef jsonrpc::Client JsonRPCClient;


enum FirewallMode
{
	Disabled = 0,
	WhenConnected = 1,
	AlwaysOn = 2,
};

enum FirewallFlags
{
	Nothing = 0,
	BlockIPv4 = 0x1, // temporary
	BlockIPv6 = 0x2, // temporary
};
static inline FirewallFlags operator|(FirewallFlags a, FirewallFlags b) { return (FirewallFlags)(+a | +b); }
static inline FirewallFlags& operator|=(FirewallFlags& a, FirewallFlags b) { return a = (a | b); }


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

	void OnOpenVPNProcessExited(OpenVPNProcess* process);

protected:
	typedef websocketpp::connection_hdl Connection;
	typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

	void SendToClient(Connection con, const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data);
	void OnFirstClientConnected();
	void OnLastClientDisconnected();
	void OnReceiveMessage(Connection con, WebSocketServer::message_ptr msg);
	void OnStateChanged();

	jsonrpc::Value::Struct RPC_getState();
	// Instruct the daemon to connect to the specified server. If already connected,
	// this instructs the daemon to seamlessly switch over to a different server.
	bool RPC_connect(const jsonrpc::Value::Struct& params);
	void RPC_disconnect();
	bool RPC_setFirewall(const jsonrpc::Value::Struct& params);

	WebSocketServer _ws_server;
	ConnectionList _connections;
	jsonrpc::JsonFormatHandler _json_handler;
	JsonRPCServer _rpc_server;
	JsonRPCClient _rpc_client;
	OpenVPNProcess *_process, *_next_process;
	State _state;
	FirewallMode _firewallMode;
	std::string _localIP, _remoteIP;
	int64_t _bytesReceived, _bytesSent;

protected:
	// Create a platform-specific handler around an OpenVPN process.
	// Note: the process isn't actually started until 'Run' is called on it.
	virtual OpenVPNProcess* CreateOpenVPNProcess(asio::io_service& io) = 0;
	// Ask the system for an available TCP port (for listening), preferably >= 'hint'.
	virtual int GetAvailablePort(int hint);
	// Get the identifier (for --dev) for an available TAP adapter to use.
	virtual std::string GetAvailableAdapter(int index) = 0;
	// Set the firewall/killswitch mode
	virtual bool SetFirewallSettings(bool enabled) { return false; }
};

