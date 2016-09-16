#pragma once

#include "json.h"

namespace jsonrpc {
	bool operator ==(const jsonrpc::Value& lhs, const jsonrpc::Value& rhs);
}

#define namedmember(type, name, default_value) \
	const type& name() const \
	{ \
		auto it = _map.find(#name); \
		if (it == _map.end()) return default_value; \
		return it->second.AsType<type>(); \
	} \
	type& name() \
	{ \
		auto it = _map.find(#name); \
		if (it != _map.end()) return const_cast<type&>(it->second.AsType<type>()); \
		return const_cast<type&>((_map[#name] = default_value).AsType<type>()); \
	} \
	void name(const type& copy) \
	{ \
		_map[#name] = copy; \
	} \
	void name(type&& move) \
	{ \
		_map[#name] = std::move(move); \
	}

class Settings
{
	JsonObject _map;

public:
	Settings();
	void ReadFromDisk();
	void WriteToDisk();
	void OnChanged() noexcept;

	namedmember(std::string, protocol, "udp")
	namedmember(std::string, remoteIP, "")
	namedmember(int, remotePort, 0)
	namedmember(int, mtu, 1400)
	namedmember(std::string, cipher, "AES-128-CBC")
	namedmember(jsonrpc::Value::Array, certificateAuthority, {})
	namedmember(jsonrpc::Value::Array, certificate, {})
	namedmember(jsonrpc::Value::Array, privateKey, {})

	namedmember(std::string, killswitchMode, "off")
	namedmember(bool, allowLAN, true)
	namedmember(bool, blockIPv6, true)

	namedmember(bool, runOpenVPNAsRoot, true)
};

extern Settings g_settings;
