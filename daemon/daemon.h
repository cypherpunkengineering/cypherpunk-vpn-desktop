#pragma once

#include "config.h"

#include <asio.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <jsonrpc-lean/server.h>
#include <jsonrpc-lean/client.h>

#include <set>
#include <thread>

extern class CypherDaemon* g_daemon;

class OpenVPNProcess;
typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
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
	class OpenVPNProcess* _process;

public:

	virtual OpenVPNProcess* CreateOpenVPNProcess(asio::io_service& io) { return nullptr; }
};

