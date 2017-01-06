#pragma once

#include "config.h"
#include "settings.h"

#include <asio.hpp>

#include <deque>
#include <functional>
#include <map>
#include <thread>

class OpenVPNProcess
{
	OpenVPNProcess(const OpenVPNProcess&) = delete;
	OpenVPNProcess& operator=(const OpenVPNProcess&) = delete;
	friend class CypherDaemon;

protected:
	OpenVPNProcess(asio::io_service& io);

	asio::io_service& _io;
	asio::ip::tcp::acceptor _management_acceptor;
	asio::ip::tcp::socket _management_socket;
	std::deque<std::string> _management_write_queue;
	asio::streambuf _management_readbuf;
	std::map<std::string, std::function<void(const std::string&)>> _on_management_response;

	JsonObject _connection;
	JsonValue _connection_server;

	std::string _username;
	std::string _password;

	bool _management_signaled;

private:
	void HandleManagementWrite(const asio::error_code& error, std::size_t bytes_transferred);
	void HandleManagementReadLine(const asio::error_code& error, std::size_t bytes_transferred);

protected:
	virtual void OnManagementInterfaceResponse(const std::string& line);

public:
	virtual ~OpenVPNProcess();

public:
	void SetSettings(const JsonObject& connection_settings); 
	int StartManagementInterface();
	void StopManagementInterface();
	void SendManagementCommand(std::string cmd);
	void OnManagementResponse(const std::string& prefix, std::function<void(const std::string&)> callback);

	bool IsSameServer(const JsonObject& settings);
	static bool SettingRequiresReconnect(const std::string& name);

	// Request clean shutdown (first via management interface, calls Kill if that fails)
	void Shutdown();

	virtual void Run(const std::vector<std::string>& params) = 0;
	virtual void Kill() = 0;
	// Asynchronously wait for process termination
	virtual void AsyncWait(std::function<void(const asio::error_code&)> cb) = 0;

	// Flag to indicate the user should reconnect to a new server for their settings to take effect.
	bool stale;
	// Number of reconnection to tolerate before we should treat as disconnected. Initally set to the number of <connection> entries
	size_t connection_retries_left;
};
