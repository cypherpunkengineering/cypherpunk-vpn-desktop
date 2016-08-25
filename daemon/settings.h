#pragma once

#include <jsonrpc-lean/value.h>

namespace jsonrpc {
	bool operator ==(const jsonrpc::Value& lhs, const jsonrpc::Value& rhs);
}

struct Settings
{
	struct Connection : public jsonrpc::Value::Struct
	{
		/*
		std::string protocol{ "udp" };
		std::string remoteIP;
		unsigned int remotePort;
		unsigned int mtu{ 1400 };
		std::string cipher{ "AES-128-CBC" };
		std::vector<std::string> certificateAuthority;
		std::vector<std::string> certificate;
		std::vector<std::string> privateKey;
		*/

		Connection() {}
		Connection(const jsonrpc::Value::Struct& json) : jsonrpc::Value::Struct(json) {}

		template<typename T>
		const T& get(const char* name) const
		{
			return at(name).AsType<T>();
		}
		template<typename T>
		const T& get(const char* name, const T& default_value) const
		{
			auto it = find(name);
			if (it == end())
				return default_value;
			else
				return it->second.AsType<T>();
		}
		template<typename T>
		void set(const char* name, T&& value)
		{
			(*this)[name] = std::forward<T>(value);
		}
	} connection;

	bool runOpenVPNAsRoot;
};
