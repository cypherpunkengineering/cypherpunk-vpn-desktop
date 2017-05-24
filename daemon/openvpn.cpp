#include "config.h"

#if OS_LINUX
#include <string.h>
#endif

#include "daemon.h"
#include "openvpn.h"
#include "logger.h"
#include "path.h"

using namespace std::placeholders;

static const std::string g_connection_setting_names[] = {
	"remotePort",
	"location",
	"localPort",
	"mtu",
	"encryption",
	"overrideDNS",
	"optimizeDNS",
	"blockAds",
	"blockMalware",
	"routeDefault",
	"exemptApple",
	//"firewall",
	//"allowLAN",
	//"blockIPv6",
	"runOpenVPNAsRoot",
};

OpenVPNProcess::OpenVPNProcess(asio::io_service& io)
	: _io(io)
	, _process(Subprocess::Create(io))
	, _management_acceptor(io, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0), true)
	, _management_socket(io)
	, _state(CREATED)
	, _management_signaled(false)
	, _cypherplay(false)
	, stale(false)
{
	_process->SetStdOutListener(WEAK_CALLBACK(OnStdOut));
	_process->SetStdErrListener(WEAK_CALLBACK(OnStdErr));
}

OpenVPNProcess::~OpenVPNProcess()
{
	_process->Kill();
}

void OpenVPNProcess::CopySettings()
{
	_connection.clear();
	for (auto name : g_connection_setting_names)
	{
		auto it = g_settings.map().find(name);
		if (it != g_settings.map().end())
			_connection[name] = JsonValue(it->second);
	}
	_cypherplay = g_settings.locationFlag() == "cypherplay";
	_connection_server = g_settings.currentLocation();
	const JsonObject& login = g_account.privacy();
	_username = login.at("username").AsString();
	_password = login.at("password").AsString();
}

bool OpenVPNProcess::CompareSettings()
{
	for (auto name : g_connection_setting_names)
	{
		auto a = _connection.find(name);
		auto b = g_settings.map().find(name);
		if (a != _connection.end())
		{
			if (b == g_settings.map().end())
				return false;
			if (*a != *b)
				return false;
		}
		else if (b != g_settings.map().end())
			return false;
	}
	if (_connection_server != g_settings.currentLocation())
		return false;
	if (_cypherplay != (g_settings.locationFlag() == "cypherplay"))
		return false;
	return true;
}


int OpenVPNProcess::StartManagementInterface()
{
	int port = _management_acceptor.local_endpoint().port();

	_io.post(WEAK_LAMBDA([this]() {
		_management_write_queue.emplace_back("\n\n\n");
		_management_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		_management_acceptor.set_option(asio::ip::tcp::acceptor::linger(false, 0));
		_management_acceptor.listen(1);
		_management_acceptor.async_accept(_management_socket, WEAK_LAMBDA([this](const asio::error_code& error) {
			if (!error)
			{
				asio::async_read_until(_management_socket, _management_readbuf, '\n', WEAK_CALLBACK(HandleManagementReadLine));
				asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), WEAK_CALLBACK(HandleManagementWrite));
			}
			else
				LOG(WARNING) << error << " : " << error.message();
		}));
	}));

	return port;
}

void OpenVPNProcess::StopManagementInterface()
{
	_io.post(WEAK_LAMBDA([this]() {
		_management_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		_management_socket.close();
	}));
}

void OpenVPNProcess::SendManagementCommand(std::string cmd)
{
	cmd.push_back('\n');
	_io.post(WEAK_LAMBDA([this, cmd = std::move(cmd)]() {
		bool first = _management_write_queue.empty();
		_management_write_queue.push_back(std::move(cmd));
		if (first && _management_socket.is_open())
		{
			asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), WEAK_CALLBACK(HandleManagementWrite));
		}
	}));
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
			asio::async_write(_management_socket, asio::buffer(_management_write_queue.front()), WEAK_CALLBACK(HandleManagementWrite));
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
			asio::async_read_until(_management_socket, _management_readbuf, '\n', WEAK_CALLBACK(HandleManagementReadLine));
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

void OpenVPNProcess::OnStdOut(const asio::error_code& error, std::string line)
{
	g_daemon->OnOpenVPNStdOut(this, error, std::move(line));
}

void OpenVPNProcess::OnStdErr(const asio::error_code& error, std::string line)
{
	g_daemon->OnOpenVPNStdErr(this, error, std::move(line));
}

void OpenVPNProcess::Shutdown()
{
	_io.dispatch(SHARED_LAMBDA([this]() {
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
	}));
}

void OpenVPNProcess::Run(const std::vector<std::string>& params)
{
	std::string openvpn = use_stunnel ? GetPath(BaseDir, "daemon", "third_party", "stunnel", "openvpn-tunnel.sh") : GetFile(OpenVPNExecutable);
	_process->Run(openvpn, params);
}

void OpenVPNProcess::Kill()
{
	_process->Kill();
}

void OpenVPNProcess::AsyncWait(std::function<void(const asio::error_code& error)> cb)
{
	_process->AsyncWait([cb = std::move(cb)](const asio::error_code& error, Subprocess::Result result) { cb(error); });
}
