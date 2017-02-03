#pragma once

#include "util.h"

#include <functional>
#include <map>
#include <string>
#include <unordered_map>

#include <jsonrpc-lean/jsonwriter.h>
#include <jsonrpc-lean/value.h>

typedef jsonrpc::Value JsonValue;
typedef jsonrpc::Value::Struct JsonObject;
typedef jsonrpc::Value::Array JsonArray;
typedef jsonrpc::Writer JsonWriter;

namespace jsonrpc {
	bool operator ==(const JsonValue& lhs, const JsonValue& rhs);
	bool operator !=(const JsonValue& lhs, const JsonValue& rhs);
}

JsonObject ReadJsonFile(const std::string& path);
void WriteJsonFile(const std::string& path, const JsonObject& obj);


// Any container for natively stored JSON members
class NativeJsonObjectContext {};

// Track a mapping between JSON object members and some native representation
class NativeJsonObjectMapping : public noncopyable
{
	typedef NativeJsonObjectContext Context;
	struct MemberInfo
	{
		typedef JsonValue Serialize(const Context* context);
		typedef void Deserialize(JsonValue&& data, Context* context);
		typedef bool Exists(const Context* context);
		typedef bool Reset(Context* context);

		// Name of member
		std::string name;
		// Convert native representation of member to a JsonValue
		std::function<Serialize> serialize;
		// Convert a JsonValue to the native representation of member
		std::function<Deserialize> deserialize;
		// Determine if member should be present in JSON
		std::function<Exists> exists;
		// Reset native representation to default value
		std::function<Reset> reset;
		// Hint that member should be serialized to disk
		bool persistent;

		MemberInfo(const MemberInfo&) = delete;
		MemberInfo(MemberInfo&&) = default;
		MemberInfo& operator=(const MemberInfo&) = delete;
	};
	std::unordered_map<std::string, MemberInfo> _members;
public:
	void AddMember(MemberInfo member);
	const MemberInfo& GetMember(const std::string& name);

	bool SerializeMember(const std::string& name, JsonValue& target, const Context* context);
	bool SerializeMember(const std::string& name, JsonObject& container, const Context* context);
	bool SerializeMember(const std::string& name, JsonWriter& writer, const Context* context);
	//bool DeserializeMember(const std::string& name, const JsonValue& value, Context* context);
	bool DeserializeMember(const std::string& name, JsonValue&& value, Context* context);

	void SerializeAllMembers(JsonObject& container, const Context* context);
	void SerializeAllMembers(JsonWriter& writer, const Context* context);
	void DeserializeAllMembers(const JsonObject& source, const Context* context);
	void DeserializeAllMembers(JsonObject&& source, const Context* context) { DeserializeAllMembersAndConsume(source, context); }
	void DeserializeAllMembersAndConsume(JsonObject& source, const Context* context);
};







/*

template<class Derived>
class NativeJsonObject
{
protected:
	std::function<void(std::string)> _on_changed;
	unordered_map<std::string, JsonValue> _unknown;
	bool _keep_unknown;

	void RegisterMember(std::string name, std::function<MemberInfo::Serializer> serializer, std::function<MemberInfo::Deserializer> deserializer, bool persistent = true);
	template<class Derived, typename T>
	void RegisterMember(std::string name, T Derived::* member, bool persistent = true)
	{
		RegisterMember(
			name,
			[=](const NativeJsonObject* self) {
				return JsonValue(static_cast<const Derived*>(self)->*member);
			},
			[=](NativeJsonObject* self, JsonValue&& data) {
				(static_cast<Derived*>(self)->*member) = data.AsType<T>();
			},
			persistent);
	}
	template<class Derived, typename T>
	void RegisterMember(std::string name, T Derived::* member, std::function<JsonValue(const T&)> to_json, std::function<void(T&, JsonValue&&)> from_json, bool persistent = true)
	{
		RegisterMember(
			name,
			[=](const NativeJsonObject* self) {
				return to_json(static_cast<const Derived*>(self)->*member);
			},
			[=](NativeJsonObject* self, JsonValue&& data) {
				from_json(static_cast<Derived*>(self)->*member, data);
			},
			persistent);
	}
	template<class Derived, typename T>
	void RegisterMember(std::string name, T Derived::* member, std::function<JsonValue(const T&)> to_json, std::function<T(JsonValue&&)> from_json, bool persistent = true)
	{
		RegisterMember(name, member, to_json, [=](T& value, JsonValue&& data) { value = from_json(value); }, persistent);
	}

public:
	JsonValue SerializeMember(std::string name);
	void SerializeMember(std::string name, JsonObject& container);
	void DeserializeMember(std::string name, JsonValue&& value);

	void Read(JsonObject&& src);
	void Write(JsonObject& dst);
	void Write(JsonWriter& writer);
	void NativeJsonObject::ToJson(std::function<void(const char* data, size_t size)> cb);
	std::string ToJson();
};
*/
