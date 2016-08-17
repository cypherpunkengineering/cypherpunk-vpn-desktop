#pragma once

#include "config.h"
#include "logger.h"

#include <asio.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

template <typename names>
class websocket_logger
{
	typedef websocketpp::log::level level;
	level _levels;
public:
	websocket_logger(level level, websocketpp::log::channel_type_hint::value hint) : _levels(level) {}

	static LogLevel GetLogLevel(level level, websocketpp::log::alevel* dummy)
	{
		switch (level)
		{
		case websocketpp::log::alevel::fail:
			return LEVEL_WARNING;
		case websocketpp::log::alevel::connect:
		case websocketpp::log::alevel::disconnect:
			return LEVEL_INFO;
		default:
			return LEVEL_VERBOSE;
		}
	}
	static LogLevel GetLogLevel(level level, websocketpp::log::elevel* dummy)
	{
		switch (level)
		{
		case websocketpp::log::elevel::fatal:
			return LEVEL_ERROR;
		case websocketpp::log::elevel::rerror:
		case websocketpp::log::elevel::warn:
			return LEVEL_WARNING;
		case websocketpp::log::elevel::info:
		case websocketpp::log::elevel::library:
			return LEVEL_INFO;
		default:
			return LEVEL_VERBOSE;
		}
	}

	void set_channels(level channels) {
		if (channels == names::none)
			_levels = names::none;
		else
			_levels |= channels;
	}

	void clear_channels(level channels) {
		_levels &= ~channels;
	}

	template<typename T>
	void write(level channel, T&& msg) {
		LogLevel l = GetLogLevel(channel, (names*)nullptr);
		LOG_EX(PrefixLogWriter<LogWriter>, l, dynamic_test(channel)) << '[' << names::channel_name(channel) << "] " << std::forward<T>(msg);
	}

	constexpr bool static_test(level channel) const {
		return true;
	}

	bool dynamic_test(level channel) const {
		return (_levels & channel) != 0;
	}
};

struct asio_with_custom_logging : public websocketpp::config::asio
{
	typedef websocket_logger<websocketpp::log::elevel> elog_type;
	typedef websocket_logger<websocketpp::log::alevel> alog_type;

	struct transport_config : public websocketpp::config::asio::transport_config {
		typedef asio_with_custom_logging::alog_type alog_type;
		typedef asio_with_custom_logging::elog_type elog_type;
	};

	typedef websocketpp::transport::asio::endpoint<transport_config>
		transport_type;
};

typedef websocketpp::server<asio_with_custom_logging> WebSocketServer;
