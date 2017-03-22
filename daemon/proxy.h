#pragma once

#include "util.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

class TLSProxy
{
public:
	TLSProxy(io_service& io) : _io(io), _ssl(asio::ssl::context::tlsv12_client) {}

	void Connect(const std::string& server) {}
	void Disconnect() {}

private:
	asio::ssl::context _ssl;
};
