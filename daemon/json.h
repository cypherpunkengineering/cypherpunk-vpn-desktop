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

#include <set>

namespace json {

class JsonValue
{
	template<typename ...Ts> struct empty_template {};
	template<typename T, typename _ = void> struct is_iterable : public std::false_type {};
	template<typename T> struct is_iterable<T, std::conditional_t<false, empty_template<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>, void>> : public std::true_type {};

public:
	typedef struct UndefinedType {} Undefined;
	typedef std::nullptr_t Null;
	typedef bool Boolean;
	typedef double Double;
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
		TYPE_NUMBER = 0x04,
		TYPE_DOUBLE = TYPE_NUMBER | 0x00,
		TYPE_INT32 = TYPE_NUMBER | 0x01,
		TYPE_STRING = 0x08,
		TYPE_OBJECT = 0x10,
		TYPE_ARRAY = TYPE_OBJECT | 0x01,

		TYPE_FROZEN = 0x80, // This JsonValue insance must not change type (e.g. this is actually a subclass with type guarantees)
		TYPE_FLAGS = (TYPE_FROZEN),
		TYPE_MASK = 0xFF ^ TYPE_FLAGS,
	};

private:
	template<typename T> static auto forward_begin(std::remove_reference_t<T>& iterable) -> decltype(std::begin(iterable)) { return std::begin(iterable); }
	template<typename T> static auto forward_begin(std::remove_reference_t<T>&& iterable) -> decltype(std::begin(iterable)) { return std::make_move_iterator(std::begin(iterable)); }
	template<typename T> static auto forward_end(std::remove_reference_t<T>& iterable) -> decltype(std::end(iterable)) { return std::end(iterable); }
	template<typename T> static auto forward_end(std::remove_reference_t<T>&& iterable) -> decltype(std::make_move_iterator(std::end(iterable))) { return std::make_move_iterator(std::end(iterable)); }

public:
	constexpr JsonValue() : _type(TYPE_UNDEFINED), _number(0) {}
	constexpr JsonValue(const Undefined&) : _type(TYPE_UNDEFINED), _number(0) {}
	constexpr JsonValue(const Null&) : _type(TYPE_NULL), _number(0) {}
	constexpr JsonValue(bool value) : _type(TYPE_BOOLEAN), _boolean(value) {}
	constexpr JsonValue(int value) : _type(TYPE_INT32), _int32(value) {}
	constexpr JsonValue(double value) : _type(TYPE_DOUBLE), _number(value) {}
	JsonValue(String value) : _type(TYPE_STRING), _string(new String(std::move(value))) {}
	JsonValue(Object value) : _type(TYPE_OBJECT), _object(new Object(std::move(value))) {}
	JsonValue(Array value) : _type(TYPE_ARRAY), _array(new Array(std::move(value))) {}
	template<typename ...Args> JsonValue(Args&& ...args) : JsonValue() { Construct(std::forward<Args>(args)...); }

	JsonValue(const JsonValue& copy) : JsonValue() { operator=(copy); }
	JsonValue(JsonValue&& move) : JsonValue() { operator=(std::move(move)); }

	~JsonValue() { Reset(); }

private:
	Type SetType(Type type)
	{
		if (!CanChangeType(type))
			throw std::invalid_argument("Attempted to change type of a typed/frozen JsonValue");
		Type old = type;
		_type = (Type)((_type & TYPE_FLAGS) | (type &~ TYPE_FLAGS));
		return old;
	}

	void Construct() { SetType(TYPE_UNDEFINED); }
	void Construct(const Undefined&) { SetType(TYPE_UNDEFINED); }
	void Construct(const Null&) { SetType(TYPE_NULL); }
	Boolean& Construct(Boolean value) { SetType(TYPE_BOOLEAN); return _boolean = value; }
	Int32& Construct(Int32 value) { SetType(TYPE_INT32); return _int32 = value; }
	Double& Construct(Double value) { SetType(TYPE_DOUBLE); return _number = value; }
	String& Construct(const String&  value) { SetType(TYPE_STRING); return *(_string = new String(value)); }
	String& Construct(      String&& value) { SetType(TYPE_STRING); return *(_string = new String(std::move(value))); }
	Object& Construct(const Object&  value) { SetType(TYPE_OBJECT); return *(_object = new Object(value)); }
	Object& Construct(      Object&& value) { SetType(TYPE_OBJECT); return *(_object = new Object(std::move(value))); }
	Array & Construct(const Array &  value) { SetType(TYPE_ARRAY ); return *(_array  = new Array (value)); }
	Array & Construct(      Array && value) { SetType(TYPE_ARRAY ); return *(_array  = new Array (std::move(value))); }
	template<typename T> std::enable_if_t<is_iterable<T>::value> Construct(T&& iterable) { Construct(forward_begin<T>(iterable), forward_end<T>(iterable)); }
	template<typename T> std::enable_if_t<std::is_assignable<char, decltype(*std::declval<T>())>::value, String&> Construct(T&& first, T&& last) { SetType(TYPE_STRING); return *(_string = new String(std::forward<T>(first), std::forward<T>(last))); }
	template<typename T> std::enable_if_t<!std::is_assignable<char, decltype(*std::declval<T>())>::value && std::is_assignable<std::pair<const std::string, JsonValue>, decltype(*std::declval<T>())>::value, Object&> Construct(T&& first, T&& last) { SetType(TYPE_OBJECT); return *(_object = new Object(std::forward<T>(first), std::forward<T>(last))); }
	template<typename T> std::enable_if_t<!std::is_assignable<char, decltype(*std::declval<T>())>::value && std::is_assignable<JsonValue, decltype(*std::declval<T>())>::value, Array&> Construct(T&& first, T&& last) { SetType(TYPE_ARRAY); return *(_array = new Array(std::forward<T>(first), std::forward<T>(last))); }
	//String& Construct(const std::initializer_list<char>& l) { SetType(TYPE_STRING); return *(_string = new String(l)); }
	//Object& Construct(const std::initializer_list<std::pair<const std::string, JsonValue>>& l) { SetType(TYPE_OBJECT); return *(_object = new Object(l)); }
	//Array& Construct(const std::initializer_list<JsonValue>& l) { SetType(TYPE_ARRAY); return *(_array = new Array(l)); }

public:
	constexpr Type GetType() const { return (Type)(_type & TYPE_MASK); }

	bool CanChangeType() const { return (_type & (TYPE_FROZEN)) == 0; }
	bool CanChangeType(Type other) const { return !CanChangeType() && GetType() != other; }
	void Freeze() { _type = (Type)(_type | TYPE_FROZEN); }
	void Unfreeze() { _type = (Type)(_type & ~TYPE_FROZEN); }

	constexpr bool IsUndefined() const { return GetType() == TYPE_UNDEFINED; }
	constexpr bool IsNull() const { return GetType() == TYPE_NULL; }
	constexpr bool IsBoolean() const { return (GetType() & TYPE_BOOLEAN) != 0; }
	constexpr bool IsNumber() const { return (GetType() & TYPE_NUMBER) != 0; }
	constexpr bool IsDouble() const { return GetType() == TYPE_DOUBLE; }
	constexpr bool IsInt32() const { return GetType() == TYPE_INT32; }
	bool IsString() const { return GetType() == TYPE_STRING; }
	bool IsObject() const { return GetType() == TYPE_OBJECT; }
	bool IsArray() const { return GetType() == TYPE_ARRAY; }

	constexpr bool IsTrue() const { return GetType() == TYPE_BOOLEAN && _boolean; }
	constexpr bool IsFalse() const { return GetType() == TYPE_BOOLEAN && !_boolean; }
	constexpr bool IsTruthy() const { return operator bool(); }
	constexpr bool IsFalsy() const { return !operator bool(); }

	constexpr operator bool() const
	{
		Type type = GetType();
		if (type < TYPE_BOOLEAN) return false;
		if (type & (TYPE_NUMBER | TYPE_STRING))
		{
			return (type & TYPE_STRING) ? !_string->empty() :
				(type & 1) ? _int32 != 0 : _number != 0;
		}
		return true;
	}

	template<typename T> std::enable_if_t<std::is_assignable<String, T>::value, JsonValue&> operator=(T&& value)
	{
		if (IsString()) { *_string = std::forward<T>(value); return *this; }
		else return Reset(std::forward<T>(value));
	}
	template<typename T> std::enable_if_t<std::is_assignable<Object, T>::value, JsonValue&> operator=(T&& value)
	{
		if (IsObject()) { *_object = std::forward<T>(value); return *this; }
		else return Reset(std::forward<T>(value));
	}
	template<typename T> std::enable_if_t<std::is_assignable<Array, T>::value, JsonValue&> operator=(T&& value)
	{
		if (IsArray()) { *_array = std::forward<T>(value); return *this; }
		else return Reset(std::forward<T>(value));
	}
	JsonValue& operator=(const JsonValue& copy)
	{
		if (&copy == this)
			return *this;
		Type type = GetType(), other = copy.GetType();
		if (type == other)
		{
			switch (type)
			{
			case TYPE_STRING: *_string = *copy._string; return *this;
			case TYPE_OBJECT: *_object = *copy._object; return *this;
			case TYPE_ARRAY: *_array = *copy._array; return *this;
			default: break;
			}
		}
		else if (!CanChangeType())
			throw std::invalid_argument("Attempted to change type of a frozen JsonValue");
		else
		{
			Reset();
			switch (other)
			{
			case TYPE_BOOLEAN: _boolean = copy._boolean; break;
			case TYPE_DOUBLE: _number = copy._number; break;
			case TYPE_INT32: _int32 = copy._int32; break;
			case TYPE_STRING: _string = new String(*copy._string); break;
			case TYPE_OBJECT: _object = new Object(*copy._object); break;
			case TYPE_ARRAY: _array = new Array(*copy._array); break;
			default: break;
			}
			SetType(copy._type);
		}
		return *this;
	}
	JsonValue& operator=(JsonValue&& move)
	{
		if (&move == this)
			return *this;
		Type type = GetType(), other = move.GetType();
		if (!CanChangeType(other))
			throw std::invalid_argument("Attempted to change type of a frozen JsonValue");
		if (move.CanChangeType(TYPE_UNDEFINED))
		{
			// Fastest case; just steal state and leave the other as undefined
			Reset();
			switch (other)
			{
			case TYPE_BOOLEAN: _boolean = move._boolean; break;
			case TYPE_DOUBLE: _number = move._number; break;
			case TYPE_INT32: _int32 = move._int32; break;
			case TYPE_STRING: _string = move._string; break;
			case TYPE_OBJECT: _object = move._object; break;
			case TYPE_ARRAY: _array = move._array; break;
			default: break;
			}
			SetType(move.SetType(TYPE_UNDEFINED));
		}
		else if (type == other)
		{
			// Can swap underlying objects and leave the other side with empty remains
			switch (type)
			{
			case TYPE_BOOLEAN: _boolean = move._boolean; break;
			case TYPE_DOUBLE: _number = move._number; break;
			case TYPE_INT32: _int32 = move._int32; break;
			case TYPE_STRING: _string->clear(); std::swap(_string, move._string);  break;
			case TYPE_OBJECT: _object->clear(); std::swap(_object, move._object); break;
			case TYPE_ARRAY: _array->clear(); std::swap(_array, move._array); break;
			default: break;
			}
		}
		else
		{
			// No choice but to copy assign
			return operator=(static_cast<const JsonValue&>(move));
		}
		return *this;
	}
	// Fallback assignment operator if nothing else matched
	template<typename T> std::enable_if_t<!std::is_assignable<String, T>::value && !std::is_assignable<Object, T>::value && !std::is_assignable<Array, T>::value, JsonValue&> operator=(T&& value) { return Reset(std::forward<T>(value)); }

protected:
	template<typename ...Args>
	JsonValue& Reset(Args&& ...args)
	{
		Reset();
		Construct(std::forward<Args>(args)...);
		return *this;
	}
	JsonValue& Reset()
	{
		Type type = SetType(TYPE_UNDEFINED);
		if (type & (TYPE_STRING | TYPE_OBJECT))
		{
			if (type == TYPE_STRING) delete _string;
			else if (type == TYPE_OBJECT) delete _object;
			else if (type == TYPE_ARRAY) delete _array;
		}
		return *this;
	}

protected:
	Type _type;
	union
	{
		Boolean _boolean;
		Double _number;
		Int32 _int32;
		String* _string;
		Object* _object;
		Array* _array;
	};
};

template<JsonValue::Type type>
class TypedJsonValue : public JsonValue
{
protected:
	template<typename T> TypedJsonValue(T&& value) : JsonValue(std::forward<T>(value)) { Freeze(); }
	template<typename T> auto CheckType(T&& value) -> decltype(std::forward<T>(value)) { if (value.GetType() != type) throw std::invalid_argument("Attempted to change type of a frozen JsonValue"); return std::forward<T>(value); }
public:
	TypedJsonValue(const TypedJsonValue&) = default;
	TypedJsonValue(TypedJsonValue&&) = default;
	TypedJsonValue(const JsonValue& copy) : JsonValue(CheckType(copy)) { Freeze(); }
	TypedJsonValue(JsonValue&& move) : JsonValue(std::move(CheckType(move))) { Freeze(); }

	TypedJsonValue& operator=(const TypedJsonValue&) = default;
	TypedJsonValue& operator=(TypedJsonValue&&) = default;
	TypedJsonValue& operator=(const JsonValue& copy) { JsonValue::operator=(CheckType(copy)); return *this; }
	TypedJsonValue& operator=(JsonValue&& move) { JsonValue::operator=(std::move(CheckType(move))); return *this; }
};

struct JsonUndefined : public JsonValue { JsonUndefined() : JsonValue() { Freeze(); } };
struct JsonNull : public JsonValue { JsonNull() : JsonValue(nullptr) { Freeze(); } };
struct JsonBoolean : public JsonValue { JsonBoolean(Boolean value) : JsonValue(value) { Freeze(); } };
struct JsonDouble : public JsonValue { JsonDouble(Double value) : JsonValue(value) { Freeze(); } };
struct JsonInt32 : public JsonValue { JsonInt32(Int32 value) : JsonValue(value) { Freeze(); } };
struct JsonString : public JsonValue { JsonString(String value) : JsonValue(value) { Freeze(); } };
struct JsonObject : public JsonValue { JsonObject(Object value) : JsonValue(value) { Freeze(); } };
struct JsonArray : public JsonValue { JsonArray(Array value) : JsonValue(value) { Freeze(); } };

static inline void test()
{
	JsonValue u;
	JsonValue n(nullptr);
	JsonValue t(true);
	JsonValue f(false);
	JsonValue i(1);
	JsonValue d(1.0);
	JsonValue s("1");
	JsonObject o({ { "n", nullptr }, { "t", true }, { "i", 1 }, { "d", 1.0 }, { "s", "s" } });
	JsonArray a({ nullptr, true, false, 1, 1.0, "1", o });

	std::set<JsonValue> ha({ nullptr, true, false, 1, 1.0, "1", o });
	JsonValue gas(ha);

	JsonValue::String ss;
	ss = s;
}

}
