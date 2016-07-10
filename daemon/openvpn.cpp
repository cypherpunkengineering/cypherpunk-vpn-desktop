#include "openvpn.h"

#include <iostream>

using namespace std::placeholders;

void OpenVPNProcess::StartManagementInterface(const asio::ip::tcp::endpoint& endpoint)
{
	asio::streambuf readbuf;

	_management_socket.async_connect(endpoint, [=, &readbuf](const asio::error_code& error) {
		if (!error)
		{
			asio::async_read_until(_management_socket, readbuf, '\n', [=, &readbuf](const asio::error_code& error, std::size_t bytes_transferred) {
				if (!error)
				{
					std::istream is(&readbuf);
					std::string line;
					std::getline(is, line);
					std::cout << line;
				}
			});
		}
	});

	asio::error_code error;
	_management_thread = std::thread([=, &error]() { _io.run(error); });
}

void OpenVPNProcess::StopManagementInterface()
{
	_io.post([=]() {
		_management_socket.close();
		_io.stop();
	});
	_management_thread.join();
}

void OpenVPNProcess::SendManagementCommand(const std::string& cmd)
{
	_io.post([=, cmd = cmd]() {
		bool first = _management_write_queue.empty();
		_management_write_queue.push_back(cmd);
		if (first)
		{
			asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), std::bind(&OpenVPNProcess::HandleManagementWrite, this, _1, _2));
		}
	});
}

void OpenVPNProcess::HandleManagementWrite(const asio::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
		_management_write_queue.pop_front();
		if (!_management_write_queue.empty())
		{
			asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), std::bind(&OpenVPNProcess::HandleManagementWrite, this, _1, _2));
		}
	}
}
