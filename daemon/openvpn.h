#pragma once

#include "config.h"

#include <asio.hpp>

#include <deque>
#include <functional>
#include <map>
#include <thread>

class OpenVPNProcess
{
	OpenVPNProcess(const OpenVPNProcess&) = delete;
	OpenVPNProcess& operator=(const OpenVPNProcess&) = delete;

protected:
	OpenVPNProcess(asio::io_service& io) : _io(io), _management_socket(io) {}

	asio::io_service& _io;
	asio::ip::tcp::socket _management_socket;
	std::thread _management_thread;
	std::deque<std::string> _management_write_queue;
	asio::streambuf _management_readbuf;
	std::map<std::string, std::function<void(const std::string&)>> _on_management_response;

private:
	void HandleManagementWrite(const asio::error_code& error, std::size_t bytes_transferred);
	void HandleManagementReadLine(const asio::error_code& error, std::size_t bytes_transferred);

protected:
	virtual void OnManagementInterfaceResponse(const std::string& line);

public:
	virtual ~OpenVPNProcess();

public:
	void StartManagementInterface(const asio::ip::tcp::endpoint& endpoint);
	void StopManagementInterface();
	void SendManagementCommand(const std::string& cmd);
	void OnManagementResponse(const std::string& prefix, std::function<void(const std::string&)> callback);

	virtual void Run(const std::vector<std::string>& params) = 0;
	virtual void Kill() = 0;
};
