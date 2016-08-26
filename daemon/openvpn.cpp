#include "config.h"

#include "openvpn.h"
#include "logger.h"

using namespace std::placeholders;

OpenVPNProcess::~OpenVPNProcess()
{

}

void OpenVPNProcess::StartManagementInterface(const asio::ip::tcp::endpoint& endpoint)
{
	_management_write_queue.emplace_back("\n\n\n");
	_management_socket.async_connect(endpoint, [this](const asio::error_code& error) {
		if (!error)
		{
			asio::async_read_until(_management_socket, _management_readbuf, '\n', std::bind(&OpenVPNProcess::HandleManagementReadLine, this, _1, _2));
			asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), std::bind(&OpenVPNProcess::HandleManagementWrite, this, _1, _2));
		}
		else
			LOG(WARNING) << error << " : " << error.message();
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

void OpenVPNProcess::OnManagementResponse(const std::string& prefix, std::function<void(const std::string&)> callback)
{
	_on_management_response[prefix] = callback;
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
	else
		LOG(WARNING) << error << " : " << error.message();
}

void OpenVPNProcess::HandleManagementReadLine(const asio::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
		std::istream is(&_management_readbuf);
		std::string line;
		std::getline(is, line);
		if (line.size() > 0 && *line.crbegin() == '\r')
			line.pop_back();
		OnManagementInterfaceResponse(line);

		asio::async_read_until(_management_socket, _management_readbuf, '\n', std::bind(&OpenVPNProcess::HandleManagementReadLine, this, _1, _2));
	}
	else
		LOG(WARNING) << error << " : " << error.message();
}

void OpenVPNProcess::OnManagementInterfaceResponse(const std::string& line)
{
	LOG(INFO) << "[MGMT] " << line;
	if (line.size() > 0)
	{
		if (line[0] == '>')
		{
			auto sep = line.find(':');
			if (sep != line.npos)
			{
				auto prefix = line.substr(1, sep - 1);
				auto it = _on_management_response.find(prefix);
				if (it != _on_management_response.end())
				{
					it->second(line.substr(sep + 1));
				}
			}
		}
		//else (line.)
	}
}

bool OpenVPNProcess::IsSameServer(const Settings::Connection& connection)
{
	return connection == _connection;
}
