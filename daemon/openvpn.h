#pragma once

#include "config.h"

#include <asio.hpp>

#include <deque>
#include <thread>

class OpenVPNProcess
{
	OpenVPNProcess(const OpenVPNProcess&) = delete;
	OpenVPNProcess& operator=(const OpenVPNProcess&) = delete;

protected:
	OpenVPNProcess(asio::io_service& io) : _io(io), _management_socket(io) {}

	void StartManagementInterface(const asio::ip::tcp::endpoint& endpoint);
	void StopManagementInterface();

	asio::io_service& _io;
	asio::ip::tcp::socket _management_socket;
	std::thread _management_thread;
	std::deque<std::string> _management_write_queue;

private:
	void HandleManagementWrite(const asio::error_code& error, std::size_t bytes_transferred);

public:
	virtual ~OpenVPNProcess() {}

public:
	void SendManagementCommand(const std::string& cmd);

	virtual void Run(const std::vector<std::string>& params) = 0;
	virtual void Kill() = 0;
};
