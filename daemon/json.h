#pragma once

#include "util.h"

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <jsonrpc-lean/jsonwriter.h>
#include <jsonrpc-lean/value.h>

typedef jsonrpc::Value            JsonValue;
typedef jsonrpc::Value::Undefined JsonUndefined;
typedef jsonrpc::Value::Null      JsonNull;
typedef jsonrpc::Value::Boolean   JsonBoolean;
typedef jsonrpc::Value::Double    JsonDouble;
typedef jsonrpc::Value::Int32     JsonInt32;
typedef jsonrpc::Value::String    JsonString;
typedef jsonrpc::Value::Object    JsonObject;
typedef jsonrpc::Value::Array     JsonArray;

typedef jsonrpc::JsonWriter       JsonWriter;

std::string ToJson(const JsonValue& value);
JsonObject ReadJsonFile(const std::string& path);
void WriteJsonFile(const std::string& path, const JsonObject& obj);


#define JsonField(type, name, default_value) \
	private: \
	type& _field_##name = InitializeField<type>(#name, default_value); \
	public: \
	type& name() { return _field_##name; } \
	const type& name() const { return _field_##name; } \
	template<typename T> type& name(T&& value) { _field_##name = std::forward<T>(value); if (_on_changed) _on_changed(#name); return _field_##name; }

class NativeJsonObject : protected JsonObject
{
protected:
	template<typename T> T& InitializeField(const char* name, const T& default_value)
	{
		JsonValue& v = JsonObject::operator[](name);
		v = JsonValue(default_value);
		v.Freeze();
		_fields.insert(name);
		return v.AsType<T>();
	}
	template<typename T> T& InitializeField(const char* name, T&& default_value)
	{
		JsonValue& v = JsonObject::operator[](name);
		v = JsonValue(std::move(default_value));
		v.Freeze();
		_fields.insert(name);
		return v.AsType<T>();
	}
	std::function<void(const char*)> _on_changed;
	std::unordered_set<std::string> _fields;
public:
	using JsonObject::JsonObject;

	std::function<void(const char*)> SetOnChangedHandler(std::function<void(const char*)> cb) { return std::exchange(_on_changed, cb); }
	void OnChanged(const char* key) { if (_on_changed) _on_changed(key); }

	const JsonValue& Set(const std::string& key, const JsonValue& value) { JsonValue& result = JsonObject::operator[](key) = value; OnChanged(key.c_str()); return result; }
	const JsonValue& Set(const std::string& key, JsonValue&& value) { JsonValue& result = JsonObject::operator[](key) = std::move(value); OnChanged(key.c_str()); return result; }

	const_iterator begin() const { return JsonObject::begin(); }
	const_iterator end() const { return JsonObject::end(); }
	const JsonObject& map() { return *this; }
	JsonValue& at(const std::string& name) { return JsonObject::at(name); }
	const JsonValue& at(const std::string& name) const { return JsonObject::at(name); }
	JsonValue& operator[](const std::string& name) { return JsonObject::operator[](name); }

	bool ReadFromDisk(const std::string& path, const char* type = "setting");
	bool WriteToDisk(const std::string& path, const char* type = "setting") const;

	void RemoveUnknownFields();
};
