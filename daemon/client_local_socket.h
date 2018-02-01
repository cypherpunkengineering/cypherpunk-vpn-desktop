#pragma once

#include "config.h"

#include "client.h"


template<class Acceptor, class Endpoint, class Socket>
class LocalSocketClientInterface : public ClientInterface
{
protected:
	typedef Acceptor acceptor_t;
	typedef Endpoint endpoint_t;
	typedef Socket socket_t;

	acceptor_t _acceptor;
	struct Connection
	{
		Connection(socket_t socket, ClientConnectionHandle handle) : socket(std::move(socket)), read_buffer(1048576), size(0), handle(handle) {}

		socket_t socket;
		asio::streambuf read_buffer;
		size_t size; // size of next message
		std::deque<std::vector<uint8_t>> write_buffer;
		ClientConnectionHandle handle;
	};
	std::map<ClientConnectionHandle, std::shared_ptr<Connection>> _connections;
	uintptr_t _handle_count;

public:
	LocalSocketClientInterface(asio::io_service& io)
		: ClientInterface(io)
		, _acceptor(io)
		, _handle_count(0)
	{

	}

	virtual void Listen() override
	{
		if (_acceptor.is_open()) return;

		auto path = GetFile(LocalSocketFile, EnsureExists);
		PrepareSocketFile(path, false);
		endpoint_t endpoint(path);
		_acceptor.open(endpoint.protocol());
		_acceptor.bind(endpoint);
		_acceptor.listen();
		PrepareSocketFile(path, true);

		LOG(INFO) << "Listening on " << path;

		BeginAccept();
	}
	virtual void Stop() override
	{
		if (_acceptor.is_open())
		{
			_acceptor.close();
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
			if (connection->socket.is_open())
			{
				bool first = connection->write_buffer.empty();
				std::vector<uint8_t> buf(4 + size);
				*reinterpret_cast<uint32_t*>(&buf[0]) = (uint32_t)size;
				memcpy(&buf[4], data, size);
				connection->write_buffer.push_back(std::move(buf));
				if (first)
				{
					asio::async_write(connection->socket, asio::buffer(connection->write_buffer.front()), std::bind(THIS_FUNCTION(HandleWrite), SHARED_THIS, std::placeholders::_1, connection, std::placeholders::_2));
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
	virtual void PrepareSocketFile(const std::string& path, bool listening) {}
	virtual bool ValidatePeer(socket_t& socket) { return true; }

	void BeginAccept()
	{
		_acceptor.async_accept(SHARED_CALLBACK(HandleAccept));
	}
	void HandleAccept(const asio::error_code& error, socket_t socket)
	{
		if (!error)
		{
			if (ValidatePeer(socket))
			{
				auto handle = reinterpret_cast<ClientConnectionHandle>(++_handle_count);
				auto connection = std::make_shared<Connection>(std::move(socket), handle);
				bool first = _connections.empty();
				_connections.insert(std::make_pair(handle, connection));
				if (_listener)
				{
					if (first) _listener->OnFirstClientConnected(this);
					_listener->OnClientConnected(this, handle);
				}
				BeginRead(connection);
			}
		}
		else
		{
			if (_acceptor.is_open() || (error != asio::error::operation_aborted && error != asio::error::broken_pipe))
				LOG(WARNING) << "Client pipe accept error: " << error;
		}
		if (_acceptor.is_open())
			BeginAccept();
	}
	void BeginRead(std::shared_ptr<Connection> connection)
	{
		connection->socket.async_read_some(connection->read_buffer.prepare(65536), std::bind(THIS_FUNCTION(HandleRead), SHARED_THIS, std::placeholders::_1, connection, std::placeholders::_2));
	}
	void HandleRead(const asio::error_code& error, std::shared_ptr<Connection> connection, size_t bytes_transferred)
	{
		if (!error)
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
			if (connection->socket.is_open())
			{
				BeginRead(std::move(connection));
			}
		}
		else
		{
			if (error != asio::error::operation_aborted && error != asio::error::broken_pipe)
				LOG(ERROR) << "Client pipe read error: " << error;
			Disconnect(connection, true);
		}
	}
	void HandleWrite(const asio::error_code& error, std::shared_ptr<Connection> connection, size_t bytes_transferred)
	{
		if (!error)
		{
			connection->write_buffer.pop_front();
			if (!connection->write_buffer.empty() && connection->socket.is_open())
			{
				asio::async_write(connection->socket, asio::buffer(connection->write_buffer.front()), std::bind(THIS_FUNCTION(HandleWrite), SHARED_THIS, std::placeholders::_1, connection, std::placeholders::_2));
			}
		}
		else
		{
			if (error != asio::error::operation_aborted && error != asio::error::broken_pipe)
				LOG(WARNING) << "Client pipe write error: " << error;
			Disconnect(connection, true);
		}
	}
	void Disconnect(std::shared_ptr<Connection> connection, bool notify_immediately)
	{
		auto handle = notify_immediately ? std::exchange(connection->handle, nullptr) : nullptr;
		bool last = _connections.erase(handle) && _connections.empty();
		asio::error_code ignored;
		connection->socket.close(ignored);
		if (handle && _listener)
		{
			_listener->OnClientDisconnected(this, handle);
			if (last) _listener->OnLastClientDisconnected(this);
		}
	}
};
