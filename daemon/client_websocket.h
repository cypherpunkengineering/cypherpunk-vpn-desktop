#pragma once

#include "client.h"
#include "path.h"
#include "util.h"
#include "websocketpp.h"


class WebSocketClientInterface : public ClientInterface
{
protected:
	WebSocketServer _ws_server;
	std::map<ClientConnectionHandle, websocketpp::connection_hdl> _outer_handle_map;
	std::map<websocketpp::connection_hdl, ClientConnectionHandle, std::owner_less<websocketpp::connection_hdl>> _inner_handle_map;
	uintptr_t _outer_handle_count;
	AutoDeleteFile _port_file;

public:
	WebSocketClientInterface(asio::io_service& io)
		: ClientInterface(io), _outer_handle_count(0), _port_file(GetFile(DaemonPortFile))
	{
		using namespace websocketpp::log;
		_ws_server.clear_access_channels(alevel::all);
		_ws_server.clear_error_channels(elevel::all);
		_ws_server.set_error_channels(elevel::fatal | elevel::rerror | elevel::warn);
#ifdef _DEBUG
		_ws_server.set_access_channels(alevel::access_core);
		_ws_server.set_error_channels(elevel::info | elevel::library);
#endif
		_ws_server.set_open_handler([this](websocketpp::connection_hdl c) {
			auto outer_handle = reinterpret_cast<ClientConnectionHandle>(++_outer_handle_count);
			bool first = _outer_handle_map.empty();
			_outer_handle_map.emplace(outer_handle, c);
			_inner_handle_map.emplace(c, outer_handle);
			if (_listener)
			{
				if (first) _listener->OnFirstClientConnected(this);
				_listener->OnClientConnected(this, outer_handle);
			}
		});
		_ws_server.set_close_handler([this](websocketpp::connection_hdl c) {
			auto outer_handle = _inner_handle_map.at(c);
			_outer_handle_map.erase(outer_handle);
			_inner_handle_map.erase(c);
			bool last = _outer_handle_map.empty();
			if (_listener)
			{
				_listener->OnClientDisconnected(this, outer_handle);
				if (last) _listener->OnLastClientDisconnected(this);
			}
		});
		_ws_server.set_message_handler([this](websocketpp::connection_hdl c, WebSocketServer::message_ptr msg) {
			auto outer_handle = _inner_handle_map.at(c);
			if (_listener) _listener->OnClientMessageReceived(this, outer_handle, msg->get_payload());
		});
		_ws_server.init_asio(&_io);
	}

	virtual void Listen() override
	{
		_ws_server.set_reuse_addr(true);
		try
		{
			_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 9337));
		}
		catch (...)
		{
			// Debug builds can't run on anything but 9337; fail.
			if (!IsInstalled()) throw;

			// Failed to open default port 9337, use any port instead
			_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0));
		}
		_ws_server.start_accept();

		asio::error_code ec;
		auto endpoint = _ws_server.get_local_endpoint(ec);
		asio::detail::throw_error(ec, "get_local_endpoint");
		int port = endpoint.port();

		_port_file.Open(GetFile(DaemonPortFile));
		if (!_port_file || fprintf(_port_file, "%d", port) < 0)
		{
			LOG(ERROR) << "Unable to write port file";
			if (port != 9337)
			{
				throw std::system_error(std::make_error_code(std::errc::io_error), "Unable to write port file");
			}
		}
		fflush(_port_file);
		LOG(DEBUG) << "Listening on port " << port;
	}
	virtual void Stop() override
	{
		_port_file.Close();
		if (_ws_server.is_listening())
		{
			std::error_code ec;
			_ws_server.stop_listening(ec);
			// ignore errors
		}
	}
	virtual void Send(ClientConnectionHandle client, const char* data, size_t size) override
	{
		auto inner_handle = _outer_handle_map.at(client);
		_ws_server.send(inner_handle, data, size, websocketpp::frame::opcode::TEXT);
	}
	virtual void Disconnect(ClientConnectionHandle client) override
	{
		auto inner_handle = _outer_handle_map.at(client);
		_ws_server.pause_reading(inner_handle);
		_ws_server.close(inner_handle, websocketpp::close::status::going_away, "");
	}
};
