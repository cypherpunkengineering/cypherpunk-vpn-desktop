#pragma once

#include "config.h"

#include "client_local_socket.h"
#include "win.h"

#include <map>

#include <asio.hpp>

#include <windows.h>
#include <sddl.h>


namespace asio {
namespace windows {
namespace named_pipe {

template<typename Protocol>
class basic_endpoint
{
protected:
	std::string _path;

public:
	typedef Protocol protocol_type;

	basic_endpoint(std::string path) : _path(std::move(path)) {}

	protocol_type protocol() const { return protocol_type(); }
	const std::string& path() const { return _path; }
};

template<typename Protocol>
class basic_acceptor
{
public:
	typedef Protocol protocol_type;
	typedef typename Protocol::endpoint endpoint_type;
	typedef typename Protocol::socket socket_type;

protected:
	asio::io_service& _io;
	std::tstring _path;
	protocol_type _protocol;
	PSECURITY_DESCRIPTOR _psd;
	socket_type _accept_pipe;
	bool _open, _listening;
	bool _reuse_addr;

public:
	basic_acceptor(asio::io_service& io)
		: _io(io)
		, _psd(NULL)
		, _accept_pipe(io)
		, _open(false)
		, _listening(false)
		, _reuse_addr(false)
	{
		// Define the SDDL for the security descriptor.
		LPCTSTR ddl = L"D:"         // Discretionary ACL
			L"(A;OICI;GRGW;;;AU)"   // Allow read/write to authenticated users
			L"(A;OICI;GA;;;BA)";    // Allow full control to administrators

		WIN_CHECK_IF_FALSE(ConvertStringSecurityDescriptorToSecurityDescriptor, (ddl, SDDL_REVISION_1, &_psd, NULL));
	}
	~basic_acceptor()
	{
		if (_psd) LocalFree(_psd);
	}

	void open(const protocol_type& protocol)
	{
		_protocol = protocol;
		_open = true;
	}
	bool is_open() const
	{
		return _open;
	}
	void close()
	{
		if (_accept_pipe.is_open())
		{
			_accept_pipe.cancel();
		}
		_listening = false;
		_open = false;
	}
	void bind(const endpoint_type& endpoint)
	{
		_path = convert<TCHAR>(endpoint.path());
	}
	void listen()
	{
		_listening = true;
	}
	const endpoint_type& local_endpoint()
	{
		return _endpoint;
	}

	template<typename MoveAcceptHandler>
	void async_accept(MoveAcceptHandler&& handler)
	{
		SECURITY_ATTRIBUTES sa = {0};
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = _psd;
		sa.bInheritHandle = FALSE;

		// Create a new pipe instance to listen for connections (TODO: upper cap on connections?)
		_accept_pipe = asio::windows::stream_handle(_io, WIN_CHECK_IF_INVALID(CreateNamedPipe, (_path.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, _protocol.flags() | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, &sa)));
		// Define the overlapped completion function
		asio::windows::overlapped_ptr overlapped(_io, [this, handler = std::move(handler)](const asio::error_code& error, size_t) {
			handler(error, std::move(_accept_pipe));
		});
		// ConnectNamedPipe should always return 0 for overlapped I/O
		if (0 != ConnectNamedPipe(_accept_pipe.native_handle(), overlapped.get())) THROW_WIN32EXCEPTION(ERROR_ASSERTION_FAILURE, ConnectNamedPipe);
		// Check last error for actual result
		switch (DWORD err = GetLastError())
		{
		case ERROR_IO_PENDING:
			overlapped.release();
			break;
		case ERROR_PIPE_CONNECTED:
			overlapped.complete(asio::error_code(), 0);
			break;
		default:
			overlapped.complete(asio::error_code(err, asio::error::get_system_category()), 0);
			break;
		}
	}
};

class datagram_protocol
{
public:
	DWORD flags() const { return PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE; }

	typedef basic_endpoint<datagram_protocol> endpoint;
	typedef asio::windows::stream_handle socket;
	typedef basic_acceptor<datagram_protocol> acceptor;
};

class stream_protocol
{
public:
	DWORD flags() const { return PIPE_TYPE_BYTE | PIPE_READMODE_BYTE; }

	typedef basic_endpoint<stream_protocol> endpoint;
	typedef asio::windows::stream_handle socket;
	typedef basic_acceptor<stream_protocol> acceptor;
};

}
}
}

class WinNamedPipeClientInterface
	: public LocalSocketClientInterface<
		asio::windows::named_pipe::stream_protocol::acceptor,
		asio::windows::named_pipe::stream_protocol::endpoint,
		asio::windows::named_pipe::stream_protocol::socket>
{
public:
	WinNamedPipeClientInterface(asio::io_service& io) : LocalSocketClientInterface(io) {}
};

