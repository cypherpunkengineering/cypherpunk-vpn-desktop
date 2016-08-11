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


class CypherDaemon
{
public:
	CypherDaemon();

public:
	// The main entrypoint of the daemon; should block for the duration of the daemon.
	int Run();

	// Requests that the daemon begin shutting down. Can be called either from within
	// the daemon or from another thread.
	void RequestShutdown();

protected:
	typedef websocketpp::connection_hdl Connection;
	typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

	void SendToClient(Connection con, const std::shared_ptr<jsonrpc::FormattedData>& data);
	void SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data);
	void OnFirstClientConnected();
	void OnLastClientDisconnected();
	void OnReceiveMessage(Connection con, WebSocketServer::message_ptr msg);

	WebSocketServer _ws_server;
	ConnectionList _connections;
	jsonrpc::JsonFormatHandler _json_handler;
	JsonRPCServer _rpc_server;
	JsonRPCClient _rpc_client;
	OpenVPNProcess* _process;

protected:
	// Create a platform-specific handler around an OpenVPN process.
	// Note: the process isn't actually started until 'Run' is called on it.
	virtual OpenVPNProcess* CreateOpenVPNProcess(asio::io_service& io) = 0;
	// Ask the system for an available TCP port (for listening), preferably >= 'hint'.
	virtual int GetAvailablePort(int hint);
};

