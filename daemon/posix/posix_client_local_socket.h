#pragma once

#include "config.h"

#include "client_local_socket.h"


class PosixLocalSocketClientInterface
	: public LocalSocketClientInterface<
	asio::local::stream_protocol::acceptor,
	asio::local::stream_protocol::endpoint,
	asio::local::stream_protocol::socket>
{
public:
	PosixLocalSocketClientInterface(asio::io_service& io) : LocalSocketClientInterface(io) {}

	virtual void PrepareSocketFile(const std::string& path, bool listening) override
	{
		// TODO: consider group readable/writable, with the cypherpunk group?
		if (!listening)
			unlink(path.c_str()); // delete any socket file from a previous run
		else
			chmod(path.c_str(), 0777); // set socket to world readable/writable
	}
	virtual void Stop() override
	{
		asio::error_code error;
		auto path = _acceptor.local_endpoint(error).path();
		LocalSocketClientInterface::Stop();
		if (!error) unlink(path.c_str());
	}
	virtual bool ValidatePeer(socket_t& socket)
	{
		return true;
	}
};
