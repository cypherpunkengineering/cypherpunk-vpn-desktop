#pragma once

#include "json.h"

#include <set>

namespace jsonrpc {
	bool operator ==(const jsonrpc::Value& lhs, const jsonrpc::Value& rhs);
}

#define namedmember(type, name, default_value) \
	private: \
	const type _default_##name = default_initialize<type>(#name, default_value); \
	public: \
	const type& name() const \
	{ \
		auto it = JsonObject::find(#name); \
		if (it == JsonObject::end()) { static type def = default_value; return def; } \
		return it->second.AsType<type>(); \
	} \
	type& name() \
	{ \
		auto it = JsonObject::find(#name); \
		if (it != JsonObject::end()) return const_cast<type&>(it->second.AsType<type>()); \
		return const_cast<type&>((JsonObject::operator[](#name) = default_value).AsType<type>()); \
	} \
	void name(const type& copy) \
	{ \
		JsonObject::operator[](#name) = copy; \
	} \
	void name(type&& move) \
	{ \
		JsonObject::operator[](#name) = std::move(move); \
	}

class Settings : private JsonObject
{
	template<typename T>
	const T default_initialize(const std::string& name, T value)
	{
		auto it = JsonObject::find(name);
		if (it == JsonObject::end())
			JsonObject::insert(std::make_pair(name, value));
		return std::move(value);
	}

public:
	Settings();
	void ReadFromDisk();
	void WriteToDisk();
	void OnChanged(const std::vector<std::string>& params) noexcept;

	JsonObject& map() { return *this; }
	JsonValue& operator[](const std::string& name);

	namedmember(std::string, protocol, "udp")
	namedmember(std::string, remoteIP, "")
	namedmember(int, remotePort, 0)
	namedmember(int, mtu, 1400)
	namedmember(std::string, cipher, "AES-128-CBC")
	namedmember(jsonrpc::Value::Array, certificateAuthority, {}) // FIXME: should be hard-coded

	namedmember(jsonrpc::Value::Array, certificate, {})
	namedmember(jsonrpc::Value::Array, privateKey, {})

	namedmember(std::string, killswitchMode, "off")
	namedmember(bool, allowLAN, true)
	namedmember(bool, blockIPv6, true)

	namedmember(bool, runOpenVPNAsRoot, true)
};

extern Settings g_settings;
