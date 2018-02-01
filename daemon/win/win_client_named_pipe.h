#pragma once

#include "client.h"
#include "win.h"

#include <map>

#include <windows.h>
#include <sddl.h>


class WinNamedPipeClientInterface : public ClientInterface
{
protected:
	//asio::windows::object_handle _connect_event;
	//asio::windows::overlapped_ptr _connect_overlapped;

	struct Connection : /*public std::enable_shared_from_this<Connection>,*/ public noncopyable, public nonmovable
	{
		Connection(asio::windows::stream_handle pipe) : pipe(std::move(pipe)), size(0), handle(0), read_buffer(1048576) {}

		asio::windows::stream_handle pipe;
		asio::streambuf read_buffer;
		size_t size; // size of next message
		std::deque<std::vector<uint8_t>> write_buffer;
		ClientConnectionHandle handle;
	};
	std::map<ClientConnectionHandle, std::shared_ptr<Connection>> _connections;
	std::shared_ptr<Connection> _next_connection;
	uintptr_t _handle_count;
	bool _listening;

public:
	WinNamedPipeClientInterface(asio::io_service& io)
		: ClientInterface(io)
		//, _connect_event(io, CreateEvent(NULL, TRUE, FALSE, NULL))
		//, _connect_overlapped()
		, _handle_count(0)
		, _listening(false)
	{
		//ZeroMemory(&_connect_overlapped, sizeof(OVERLAPPED));
		//_connect_overlapped.hEvent = _connect_event.native_handle();
	}

	virtual void Listen() override
	{
		_listening = true;
		BeginAccept();
	}
	virtual void Stop() override
	{
		_listening = false;
		if (_next_connection)
		{
			Disconnect(_next_connection, false);
		}
		for (auto it = _connections.begin(); it != _connections.end(); )
		{
			auto connection = it->second;
			it = _connections.erase(it);
			Disconnect(connection, false);
		}
	}
	virtual void Send(ClientConnectionHandle client, const char* data, size_t size) override
	{
		auto it = _connections.find(client);
		if (it != _connections.end())
		{
			auto& connection = it->second;
			if (connection->pipe.is_open())
			{
				bool first = connection->write_buffer.empty();
				std::vector<uint8_t> buf(4 + size);
				*reinterpret_cast<uint32_t*>(&buf[0]) = (uint32_t)size;
				memcpy(&buf[4], data, size);
				connection->write_buffer.push_back(std::move(buf));
				if (first)
				{
					asio::async_write(connection->pipe, asio::buffer(connection->write_buffer.front()), std::bind(WEAK_CALLBACK(HandleWrite), std::placeholders::_1, connection, std::placeholders::_2));
				}
				return;
			}
		}
		LOG(WARNING) << "Attemped to send to already disconnected client";
	}
	virtual void Disconnect(ClientConnectionHandle client) override
	{
		auto it = _connections.find(client);
		if (it != _connections.end())
			Disconnect(it->second, false);
	}

protected:
	void BeginAccept()
	{
		if (!_listening || _next_connection) return;

		PSECURITY_DESCRIPTOR psd = NULL;
		SECURITY_ATTRIBUTES sa;
		ZeroMemory(&sa, sizeof(sa));

		// Define the SDDL for the security descriptor.
		LPCTSTR ddl = L"D:"         // Discretionary ACL
			L"(A;OICI;GRGW;;;AU)"   // Allow read/write to authenticated users
			L"(A;OICI;GA;;;BA)";    // Allow full control to administrators

		WIN_CHECK_IF_FALSE(ConvertStringSecurityDescriptorToSecurityDescriptor, (ddl, SDDL_REVISION_1, &psd, NULL));
		CLEANUP({ LocalFree(psd); });

		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = psd;
		sa.bInheritHandle = FALSE;

		asio::windows::overlapped_ptr overlapped(_io, std::bind(THIS_FUNCTION(HandleAccept), SHARED_THIS, std::placeholders::_1));

		auto path = GetFile(LocalSocketFile);
		LOG(DEBUG) << "Listening on " << path;

		// Create a new pipe instance to listen for connections (TODO: upper cap on connections?)
		_next_connection = std::make_shared<Connection>(asio::windows::stream_handle(_io, WIN_CHECK_IF_INVALID(CreateNamedPipe, (convert<TCHAR>(path).c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, /*PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |*/ PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, &sa))));
		// ConnectNamedPipe should always return 0 for overlapped I/O
		if (0 != ConnectNamedPipe(_next_connection->pipe.native_handle(), overlapped.get())) THROW_WIN32EXCEPTION(ERROR_ASSERTION_FAILURE, ConnectNamedPipe);
		// Check last error for actual result
		switch (DWORD err = GetLastError())
		{
		case ERROR_IO_PENDING:
			//_connect_event.async_wait(std::bind(&WinCypherDaemon::handle_accept, this, std::move(pipe), std::placeholders::_1));
			overlapped.release();
			//_connect_event.async_wait(SHARED_CALLBACK(HandleAccept));
			break;
		case ERROR_PIPE_CONNECTED:
			//_io.post(std::bind(&WinNamedPipeClientInterface::HandleAccept, SHARED_THIS, asio::error_code()));
			overlapped.complete(asio::error_code(), 0);
			break;
		default:
			// Unexpected error; post to completion callback
			overlapped.complete(asio::error_code(err, asio::error::get_system_category()), 0);
			break;
			//THROW_WIN32EXCEPTION(err, ConnectNamedPipe);
		}
	}
	void HandleAccept(const asio::error_code& error)
	{
		std::shared_ptr<Connection> connection = std::exchange(_next_connection, nullptr);
		if (error)
		{
			if (_listening || (error != asio::error::operation_aborted && error != asio::error::broken_pipe))
				LOG(WARNING) << "Named pipe accept error: " << error;
		}
		else
		{
			auto handle = reinterpret_cast<ClientConnectionHandle>(++_handle_count);
			connection->handle = handle;
			bool first = _connections.empty();
			_connections.insert(std::make_pair(handle, connection));
			if (_listener)
			{
				if (first) _listener->OnFirstClientConnected(this);
				_listener->OnClientConnected(this, handle);
			}
			BeginRead(std::move(connection));
		}
		if (_listening)
			BeginAccept();
	}
	void BeginRead(std::shared_ptr<Connection> connection)
	{
		//asio::async_read(connection->pipe, connection->read_buffer, std::bind(THIS_FUNCTION(HandleRead), SHARED_THIS, std::placeholders::_1, connection, std::placeholders::_2));
		connection->pipe.async_read_some(connection->read_buffer.prepare(65536), std::bind(THIS_FUNCTION(HandleRead), SHARED_THIS, std::placeholders::_1, connection, std::placeholders::_2));
	}
	void HandleRead(const asio::error_code& error, std::shared_ptr<Connection> connection, size_t bytes_transferred)
	{
		if (error)
		{
			if (error != asio::error::operation_aborted && error != asio::error::broken_pipe)
				LOG(ERROR) << "Client pipe read error: " << error;
			Disconnect(connection, true);
		}
		else
		{
			connection->read_buffer.commit(bytes_transferred);
			size_t bytes_available = connection->read_buffer.size();
			std::istream is(&connection->read_buffer);
			while (bytes_available > 0)
			{
				if (connection->size == 0)
				{
					if (bytes_available < 4)
						break;
					else
					{
						uint32_t size;
						is.read(reinterpret_cast<char*>(&size), 4);
						connection->size = size;
						bytes_available -= 4;
						if (connection->size == 0 || connection->size > 1048576 - 4)
						{
							LOG(WARNING) << "Invalid client message";
							Disconnect(connection, true);
							break;
						}
					}
				}
				//if (connection->size > 0)
				{
					if (bytes_available < connection->size)
						break;
					else
					{
						std::string message;
						message.resize(connection->size);
						is.read(&message[0], connection->size);
						bytes_available -= connection->size;
						connection->size = 0;
						if (_listener) _listener->OnClientMessageReceived(this, connection->handle, std::move(message));
					}
				}
			}
			BeginRead(std::move(connection));
		}
	}
	void HandleWrite(const asio::error_code& error, std::shared_ptr<Connection> connection, size_t bytes_transferred)
	{
		if (error)
		{
			if (error != asio::error::operation_aborted)
				LOG(WARNING) << "Client pipe write error: " << error;
			Disconnect(connection, true);
		}
		else
		{
			connection->write_buffer.pop_front();
			if (!connection->write_buffer.empty() && connection->pipe.is_open())
			{
				asio::async_write(connection->pipe, asio::buffer(connection->write_buffer.front()), std::bind(THIS_FUNCTION(HandleWrite), SHARED_THIS, std::placeholders::_1, connection, std::placeholders::_2));
			}
		}
	}
	void Disconnect(std::shared_ptr<Connection> connection, bool notify_immediately)
	{
		auto handle = notify_immediately ? std::exchange(connection->handle, nullptr) : nullptr;
		bool last = _connections.erase(handle) && _connections.empty();
		asio::error_code ignored;
		connection->pipe.close(ignored);
		if (handle && _listener)
		{
			_listener->OnClientDisconnected(this, handle);
			if (last) _listener->OnLastClientDisconnected(this);
		}
	}
};
