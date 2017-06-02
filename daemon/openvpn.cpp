#include "config.h"

#if OS_LINUX
#include <string.h>
#endif

#include "daemon.h"
#include "openvpn.h"
#include "logger.h"
#include "path.h"

using namespace std::placeholders;


OpenVPNProcess::OpenVPNProcess(asio::io_service& io, OpenVPNListener* listener)
	: _io(io)
	, _process(Subprocess::Create(io))
	, _management_acceptor(io, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0), true)
	, _management_socket(io)
	, _listener(listener)
	, _management_signaled(false)
{
}

OpenVPNProcess::~OpenVPNProcess()
{
	_process->Kill();
}


int OpenVPNProcess::StartManagementInterface()
{
	int port = _management_acceptor.local_endpoint().port();

	// FIXME: Need to 

	_io.post(WEAK_LAMBDA([this]() {
		_management_write_queue.emplace_back("\n\n\n");
		_management_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		_management_acceptor.set_option(asio::ip::tcp::acceptor::linger(false, 0));
		_management_acceptor.listen(1);
		// Note: Don't keep alive with shared_from_this, as that will prevent shutting down unused sockets
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

void OpenVPNProcess::HandleManagementWrite(const asio::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
		LOG(INFO) << "Wrote management line: " << _management_write_queue.front();
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

		if (line.size() > 0)
		{
			LOG_EX(LogLevel::INFO, true, Location("OpenVPN:MGMT")) << line;
			if (_listener) _listener->OnOpenVPNManagement(this, error, std::move(line));
		}

		if (_management_socket.is_open())
		{
			asio::async_read_until(_management_socket, _management_readbuf, '\n', WEAK_CALLBACK(HandleManagementReadLine));
		}
	}
	else
	{
		LOG(WARNING) << error << " : " << error.message();
		if (_listener) _listener->OnOpenVPNManagement(this, error, std::string());
	}
}

void OpenVPNProcess::OnStdOut(const asio::error_code& error, std::string line)
{
	if (_listener) _listener->OnOpenVPNStdOut(this, error, std::move(line));
}

void OpenVPNProcess::OnStdErr(const asio::error_code& error, std::string line)
{
	if (_listener) _listener->OnOpenVPNStdErr(this, error, std::move(line));
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
	_process->SetStdOutListener(WEAK_CALLBACK(OnStdOut));
	_process->SetStdErrListener(WEAK_CALLBACK(OnStdErr));

	std::string openvpn = GetFile(OpenVPNExecutable);
	_process->Run(openvpn, params);

	_process->AsyncWait(WEAK_LAMBDA([this](const asio::error_code& error, Subprocess::Result result) { if (_listener) _listener->OnOpenVPNExited(this, error); }));
}

void OpenVPNProcess::Kill()
{
	_process->Kill();
}

/*
void OpenVPNProcess::AsyncWait(std::function<void(const asio::error_code& error)> cb)
{
	_process->AsyncWait([cb = std::move(cb)](const asio::error_code& error, Subprocess::Result result) { cb(error); });
}
*/
