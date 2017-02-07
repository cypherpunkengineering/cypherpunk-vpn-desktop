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
template<class Context>
class NativeJsonObjectMapping : public noncopyable
{
public:
	struct MemberInfo
	{
		typedef JsonValue Serialize(const Context* context);
		typedef void Deserialize(JsonValue&& data, Context* context);
		typedef bool Exists(const Context* context);
		typedef void Reset(Context* context);

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

		MemberInfo(std::string name, std::function<Serialize> serialize, std::function<Deserialize> deserialize, std::function<Exists> exists, std::function<Reset> reset, bool persistent = true)
			: name(std::move(name)), serialize(std::move(serialize)), deserialize(std::move(deserialize)), exists(std::move(exists)), reset(std::move(reset)), persistent(persistent) {}
		MemberInfo(const MemberInfo&) = delete;
		MemberInfo(MemberInfo&&) = default;
		MemberInfo& operator=(const MemberInfo&) = delete;
	};
private:
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
	void DeserializeAllMembers(const JsonObject& source, Context* context);
	void DeserializeAllMembers(JsonObject&& source, Context* context) { DeserializeAllMembersAndConsume(source, context); }
	void DeserializeAllMembersAndConsume(JsonObject& source, Context* context);
};


template<class Context>
void NativeJsonObjectMapping<Context>::AddMember(MemberInfo member)
{
	std::string name = member.name;
	_members.emplace(std::pair<const std::string, MemberInfo>(std::move(name), std::move(member)));
}
template<class Context>
const typename NativeJsonObjectMapping<Context>::MemberInfo& NativeJsonObjectMapping<Context>::GetMember(const std::string& name)
{
	return _members.at(name);
}
template<class Context>
bool NativeJsonObjectMapping<Context>::SerializeMember(const std::string& name, JsonValue& target, const Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		target = JsonValue(it->second.serialize(context));
		return true;
	}
	return false;
}
template<class Context>
bool NativeJsonObjectMapping<Context>::SerializeMember(const std::string& name, JsonObject& container, const Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		container[name] = JsonValue(it->second.serialize(context));
		return true;
	}
	return false;
}
template<class Context>
bool NativeJsonObjectMapping<Context>::SerializeMember(const std::string& name, JsonWriter& writer, const Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		writer.StartStructElement(name);
		(it->second.serialize(context)).Write(writer);
		writer.EndStructElement();
		return true;
	}
	return false;
}
//template<class Context>
//bool NativeJsonObjectMapping<Context>::DeserializeMember(const std::string& name, const JsonValue& value, Context* context)
//{
//
//}
template<class Context>
bool NativeJsonObjectMapping<Context>::DeserializeMember(const std::string& name, JsonValue&& value, Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		it->second.deserialize(std::move(value), context);
		return true;
	}
	return false;
}
template<class Context>
void NativeJsonObjectMapping<Context>::SerializeAllMembers(JsonObject& container, const Context* context)
{
	for (const auto& p : _members)
	{
		container[p.first] = p.second.serialize(context);
	}
}
template<class Context>
void NativeJsonObjectMapping<Context>::SerializeAllMembers(JsonWriter& writer, const Context* context)
{
	for (const auto& p : _members)
	{
		writer.StartStructElement(p.first);
		p.second.serialize(context).Write(writer);
		writer.EndStructElement();
	}
}
template<class Context>
void NativeJsonObjectMapping<Context>::DeserializeAllMembers(const JsonObject& source, Context* context)
{
	for (const auto& p : source)
	{
		auto it = _members.find(p.first);
		if (it != _members.end())
		{
			it->second.deserialize(JsonValue(p.second), context);
		}
	}
}
template<class Context>
void NativeJsonObjectMapping<Context>::DeserializeAllMembersAndConsume(JsonObject& source, Context* context)
{
	for (const auto& p : _members)
	{
		auto it = source.find(p.first);
		if (it != source.end())
		{
			p.second.deserialize(std::move(it->second), context);
			source.erase(it);
		}
	}
}













template<class Derived>
class NativeJsonObject
{
protected:
	NativeJsonObjectMapping<Derived> _mapping;
	std::function<void(const std::string&)> _on_changed;
	std::unordered_map<std::string, JsonValue> _unknown;
	bool _keep_unknown;

	template<typename T> T initialize(T&& value, std::string name, std::function<JsonValue(const Derived*)> serialize, std::function<void(JsonValue&&, Derived*)> deserialize, std::function<bool(const Derived*)> exists, std::function<void(Derived*)> reset, bool persistent = true)
	{
		_mapping.AddMember(typename NativeJsonObjectMapping<Derived>::MemberInfo(std::move(name), std::move(serialize), std::move(deserialize), std::move(exists), std::move(reset), persistent));
		return std::forward<T>(value);
	}

public:
	NativeJsonObject(bool keep_unknown = true) : _keep_unknown(keep_unknown) {}
	void SetOnChangedListener(std::function<void(const std::string&)> on_changed) { _on_changed = on_changed; }

	bool SerializeMember(const std::string& name, JsonValue& target) { return _mapping.SerializeMember(name, target, this); }
	bool SerializeMember(const std::string& name, JsonObject& container) { return _mapping.SerializeMember(name, container, this); }
	bool SerializeMember(const std::string& name, JsonWriter& writer) { return _mapping.SerializeMember(name, writer, this); }
	//bool DeserializeMember(const std::string& name, const JsonValue& value) { return _mapping.DeserializeMember(name, value, this); }
	bool DeserializeMember(const std::string& name, JsonValue&& value) { return _mapping.DeserializeMember(name, static_cast<JsonValue&&>(value), this); }

	void SerializeAllMembers(JsonObject& container) { _mapping.SerializeAllMembers(container, this); }
	void SerializeAllMembers(JsonWriter& writer) { _mapping.SerializeAllMember(writer, this); }
	void DeserializeAllMembers(const JsonObject& source) { _mapping.DeserializeAllMembers(source, this); }
	void DeserializeAllMembers(JsonObject&& source) { _mapping.DeserializeAllMembersAndConsume(source, this); }
	void DeserializeAllMembersAndConsume(JsonObject& source) { _mapping.DeserializeAllMembersAndConsume(source, this); }

	void Read(JsonObject&& src)
	{

	}
	void Write(JsonObject& dst);
	void Write(JsonWriter& writer);
	void ToJson(std::function<void(const char* data, size_t size)> cb);
	std::string ToJson();
};




#define JsonFieldEx(type, name, default_value, serialize_expr, deserialize_expr, optional_expr) \
	type name = initialize(default_value, #name, \
		[](const auto&& self) -> JsonValue { const auto& value = self->name; return (serialize_expr); }, \
		[](JsonValue&& input, auto&& self) { auto& value = self->name; value = (deserialize_expr); }, \
		[](const auto&& self) -> bool { const auto& value = self->name; return !(optional_expr); }, \
		[](auto&& self) { self->name = default_value; })
#define JsonField(type, name, default_value) JsonFieldEx(type, name, default_value, value, input.AsType<type>(), false)

class Config : public NativeJsonObject<Config>
{
	JsonField(std::string, test, "");

};



/*
class JsonType {};

class JsonUndefined : public JsonType {};
class JsonNull : public JsonType {};
class JsonBoolean : public JsonType {};
class JsonTrue : public JsonType {};
class JsonFalse : public JsonType {};
class JsonNumber : public JsonType {};
class JsonDouble : public JsonType {};
class JsonInt32 : public JsonType {};
class JsonString : public JsonType, public std::string
class JsonObject : public JsonType, public std::unordered_map<std::string, JsonValue> {};
*/

namespace json {

class JsonUndefined
{
public:
	constexpr JsonUndefined() {}
};

class JsonValue final
{
public:
	typedef JsonUndefined Undefined;
	typedef bool Boolean;
	typedef double Number;
	typedef int Int32;
	typedef std::string String;
	typedef std::unordered_map<String, JsonValue> Object;
	typedef std::vector<JsonValue> Array;

	static constexpr Undefined undefined = Undefined();

	enum Type : unsigned char
	{
		TYPE_UNDEFINED = 0x00,
		TYPE_NULL = 0x01,
		TYPE_BOOLEAN = 0x02,
		TYPE_FALSE = TYPE_BOOLEAN | 0x00,
		// all types above this line are falsy
		TYPE_TRUE = TYPE_BOOLEAN | 0x01,
		TYPE_NUMBER = 0x04,
		TYPE_DOUBLE = TYPE_NUMBER | 0x00,
		TYPE_INT32 = TYPE_NUMBER | 0x01,
		TYPE_STRING = 0x08,
		TYPE_OBJECT = 0x10,
		TYPE_ARRAY = TYPE_OBJECT | 0x01,
	};

	constexpr JsonValue() : _type(TYPE_UNDEFINED), _number(0) {}
	constexpr JsonValue(const JsonUndefined&) : _type(TYPE_UNDEFINED), _number(0) {}
	constexpr JsonValue(std::nullptr_t) : _type(TYPE_NULL), _number(0) {}
	constexpr JsonValue(bool value) : _type(value ? TYPE_TRUE : TYPE_FALSE), _number(0) {}
	constexpr JsonValue(int value) : _type(TYPE_INT32), _int32(value) {}
	constexpr JsonValue(double value) : _type(TYPE_DOUBLE), _number(value) {}
	JsonValue(const String& value) : _type(TYPE_STRING), _string(new String(value)) {}
	JsonValue(String&& value) : _type(TYPE_STRING), _string(new String(std::move(value))) {}
	JsonValue(const Object& value) : _type(TYPE_OBJECT), _object(new Object(value)) {}
	JsonValue(Object&& value) : _type(TYPE_OBJECT), _object(new Object(std::move(value))) {}
	JsonValue(const Array& value) : _type(TYPE_ARRAY), _array(new Array(value)) {}
	JsonValue(Array&& value) : _type(TYPE_ARRAY), _array(new Array(std::move(value))) {}

	template<typename ...Args> JsonValue(Args&& ...args, std::enable_if_t<std::is_constructible<String, Args...>::value, void*> = 0) : _type(TYPE_STRING), _string(new String(std::forward<Args>(args)...)) {}

	template<typename ...Args> JsonValue(Args&& ...args, std::enable_if_t<std::is_constructible<Object, Args...>::value, void*> = 0) : _type(TYPE_OBJECT), _object(new Object(std::forward<Args>(args)...)) {}
	template<typename T> JsonValue(const std::map<std::string, T>& value) : _type(TYPE_OBJECT), _object(new Object()) { InitObject(value); }
	template<typename T> JsonValue(std::map<std::string, T>&& value) : _type(TYPE_OBJECT), _object(new Object()) { InitObject(std::move(value)); }
	template<typename T> JsonValue(const std::unordered_map<std::string, T>& value) : _type(TYPE_OBJECT), _object(new Object()) { InitObject(value); }
	template<typename T> JsonValue(std::unordered_map<std::string, T>&& value) : _type(TYPE_OBJECT), _object(new Object()) { InitObject(std::move(value)); }
	template<typename T> JsonValue(const std::initializer_list<std::pair<std::string, T>>& properties, std::enable_if_t<std::is_constructible<JsonValue, T>::value, void*> = 0) : _type(TYPE_OBJECT), _object(new Object()) { InitObject(properties); }

	template<typename ...Args> JsonValue(Args&& ...args, std::enable_if_t<std::is_constructible<Array, Args...>::value, void*> = 0) : _type(TYPE_ARRAY), _array(new Array(std::forward<Args>(args)...)) {}
	template<typename T> JsonValue(const std::vector<T>& values, std::enable_if_t<std::is_constructible<JsonValue, T>::value, void*> = 0) : _type(TYPE_ARRAY), _array(new Array()) { InitArray(values); }
	template<typename T> JsonValue(std::vector<T>&& values, std::enable_if_t<std::is_constructible<JsonValue, T>::value, void*> = 0) : _type(TYPE_ARRAY), _array(new Array()) { InitArray(std::move(values)); }

	~JsonValue()
	{
		if (_type & (TYPE_STRING | TYPE_OBJECT))
		{
			if (_type == TYPE_STRING) delete _string;
			else if (_type == TYPE_OBJECT) delete _object;
			else if (_type == TYPE_ARRAY) delete _array;
		}
		_type = TYPE_UNDEFINED;
	}

	template<typename T>
	void InitObject(T&& enumerable)
	{
		try { for (auto& p : enumerable) { _object->emplace(std::move(p)); } }
		catch (...) { Reset(); throw; }
	}
	template<typename T>
	void InitArray(T&& enumerable)
	{
		try { for (auto& v : enumerable) { _array->emplace_back(std::move(v)); } }
		catch (...) { Reset(); throw; }
	}

	bool IsUndefined() const { return _type == TYPE_UNDEFINED; }
	bool IsNull() const { return _type == TYPE_NULL; }
	bool IsBoolean() const { return (_type & TYPE_BOOLEAN) != 0; }
	bool IsTrue() const { return _type == TYPE_TRUE; }
	bool IsFalse() const { return _type == TYPE_FALSE; }

	bool IsTruthy() const { return operator bool(); }
	bool IsFalsy() const { return !operator bool(); }

	constexpr operator bool() const
	{
		if (_type < TYPE_TRUE) return false;
		if (_type & (TYPE_NUMBER | TYPE_STRING))
		{
			return (_type & TYPE_STRING) ? !_string->empty() :
				(_type & 1) ? _int32 != 0 : _number != 0;
		}
		return true;
	}

	/*
	static constexpr Type TypeOf(const JsonUndefined&) { return TYPE_UNDEFINED; }
	static constexpr Type TypeOf(const std::nullptr_t&) { return TYPE_NULL; }
	static constexpr Type TypeOf(bool value) { return value ? TYPE_TRUE : TYPE_FALSE; }
	static constexpr Type TypeOf(int) { return TYPE_INT32; }
	static constexpr Type TypeOf(double) { return TYPE_DOUBLE; }

	static constexpr Type TypeOf(const std::string&) { return TYPE_STRING; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<JsonValue, T>::value, Type> TypeOf(const std::map<std::string, T>&) { return TYPE_OBJECT; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<JsonValue, T>::value, Type> TypeOf(const std::unordered_map<std::string, T>&) { return TYPE_OBJECT; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<JsonValue, T>::value, Type> TypeOf(const std::vector<T>&) { return TYPE_ARRAY; }

	template<typename T> static constexpr std::enable_if_t<std::is_constructible<String, T>::value, Type> TypeOf(const T&) { return TYPE_STRING; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<Object, T>::value, Type> TypeOf(const T&) { return TYPE_OBJECT; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<Array, T>::value, Type> TypeOf(const T&) { return TYPE_ARRAY; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<std::map<std::string, >, T>::value, Type> TypeOf(const T&) { return TYPE_; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<JsonValue, T>::value, Type> TypeOf(const T&) { return TYPE_; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<JsonValue, T>::value, Type> TypeOf(const T&) { return TYPE_; }
	template<typename T> static constexpr std::enable_if_t<std::is_constructible<JsonValue, T>::value, Type> TypeOf(const T&) { return TYPE_; }
	*/

	template<typename T> JsonValue& operator=(T&& value) { return Reset(std::forward<T>(value)); }



protected:
	template<typename ...Args>
	JsonValue& Reset(Args&& ...args)
	{
		this->~JsonValue();
		try
		{
			new(this) JsonValue(std::forward<Args>(args)...);
		}
		catch (...)
		{
			new(this) JsonValue();
			throw;
		}
		return *this;
	}

protected:
	Type _type;
	union
	{
		Number _number;
		Int32 _int32;
		String* _string;
		Object* _object;
		Array* _array;
	};
};

}
