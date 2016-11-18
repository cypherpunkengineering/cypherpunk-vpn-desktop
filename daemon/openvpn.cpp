#include "config.h"

#if OS_LINUX
#include <string.h>
#endif

#include "openvpn.h"
#include "logger.h"

using namespace std::placeholders;

static const std::string g_connection_setting_names[] = {
	"remotePort",
	"server",
	"localPort",
	"mtu",
	"encryption",
	"firewall",
	"allowLAN",
	"blockIPv6",
	"runOpenVPNAsRoot",
};

OpenVPNProcess::OpenVPNProcess(asio::io_service& io)
	: _io(io)
	, _management_acceptor(io, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0), true)
	, _management_socket(io)
	, _management_signaled(false)
	, stale(false)
{

}

OpenVPNProcess::~OpenVPNProcess()
{

}

void OpenVPNProcess::SetSettings(const JsonObject& connection_settings)
{
	for (auto name : g_connection_setting_names)
	{
		auto it = connection_settings.find(name);
		if (it != connection_settings.end())
			_connection[name] = JsonValue(it->second);
	}
	_connection_server = JsonValue(connection_settings.at("servers").AsStruct().at(connection_settings.at("server").AsString()));
}

int OpenVPNProcess::StartManagementInterface()
{
	int port = _management_acceptor.local_endpoint().port();

	_io.post([this]() {
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
	_io.post([this]() {
		_management_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		_management_socket.close();
	});
}

void OpenVPNProcess::SendManagementCommand(std::string cmd)
{
	cmd.push_back('\n');
	_io.post([this, cmd = std::move(cmd)]() {
		bool first = _management_write_queue.empty();
		_management_write_queue.push_back(std::move(cmd));
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
	if (line.size() > 0)
	{
		LOG_EX(LogLevel::INFO, true, Location("OpenVPN:MGMT")) << line;
		if (line[0] == '>')
		{
			auto sep = line.find(':');
			if (sep != line.npos)
			{
				auto prefix = line.substr(1, sep - 1);
				auto it = _on_management_response.find(prefix);
				if (it != _on_management_response.end())
				{
					try
					{
						it->second(line.substr(sep + 1));
					}
					catch (const std::exception& e)
					{
						LOG(ERROR) << e;
					}
				}
			}
		}
	}
}

bool OpenVPNProcess::IsSameServer(const jsonrpc::Value::Struct& connection)
{
	for (auto name : g_connection_setting_names)
	{
		auto a = _connection.find(name);
		auto b = connection.find(name);
		if (a != _connection.end())
		{
			if (b == connection.end())
				return false;
			if (*a != *b)
				return false;
		}
		else if (b != connection.end())
			return false;
	}
	if (_connection_server != connection.at("servers").AsStruct().at(connection.at("server").AsString()))
		return false;
	return true;
}

bool OpenVPNProcess::SettingRequiresReconnect(const std::string& name)
{
	for (auto n : g_connection_setting_names)
		if (n == name)
			return true;
	if (name == "servers")
		return true;
	return false;
}

void OpenVPNProcess::Shutdown()
{
	_io.dispatch([this]() {
		if (!_management_signaled)
		{
			if (_management_socket.is_open())
			{
				SendManagementCommand("\nsignal SIGTERM\n");
			}
			_management_signaled = true;
		}
		else
		{
			Kill();
		}
	});
}
