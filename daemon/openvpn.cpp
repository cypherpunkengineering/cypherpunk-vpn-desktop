#include "config.h"

#include "openvpn.h"
#include "logger.h"

using namespace std::placeholders;

OpenVPNProcess::OpenVPNProcess(asio::io_service& io)
	: _io(io)
	, _management_acceptor(io, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0), true)
	, _management_socket(io)
{

}

OpenVPNProcess::~OpenVPNProcess()
{

}

int OpenVPNProcess::StartManagementInterface()
{
	int port = _management_acceptor.local_endpoint().port();

	_io.post([=]() {
		_management_write_queue.emplace_back("\n\n\n");
		_management_acceptor.listen(1);
		_management_acceptor.async_accept(_management_socket, [this](const asio::error_code& error) {
			if (!error)
			{
				asio::async_read_until(_management_socket, _management_readbuf, '\n', std::bind(&OpenVPNProcess::HandleManagementReadLine, this, _1, _2));
				asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), std::bind(&OpenVPNProcess::HandleManagementWrite, this, _1, _2));
			}
			else
				LOG(WARNING) << error << " : " << error.message();
		});
	});

	return port;
}

void OpenVPNProcess::StopManagementInterface()
{
	_io.post([=]() {
		_management_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		_management_socket.close();
	});
}

void OpenVPNProcess::SendManagementCommand(const std::string& cmd)
{
	_io.post([=, cmd = cmd]() {
		bool first = _management_write_queue.empty();
		_management_write_queue.push_back(cmd);
		if (first && _management_socket.is_open())
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
		if (!_management_write_queue.empty() && _management_socket.is_open())
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

		if (_management_socket.is_open())
		{
			asio::async_read_until(_management_socket, _management_readbuf, '\n', std::bind(&OpenVPNProcess::HandleManagementReadLine, this, _1, _2));
		}
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

bool OpenVPNProcess::IsSameServer(const jsonrpc::Value::Struct& connection)
{
	return connection == _connection;
}
