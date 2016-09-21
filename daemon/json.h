#pragma once

#include <string>
#include <jsonrpc-lean/value.h>

typedef jsonrpc::Value JsonValue;
typedef jsonrpc::Value::Struct JsonObject;
typedef jsonrpc::Value::Array JsonArray;

namespace jsonrpc {
	bool operator ==(const JsonValue& lhs, const JsonValue& rhs);
	bool operator !=(const JsonValue& lhs, const JsonValue& rhs);
}

JsonObject ReadJsonFile(const std::string& path);
void WriteJsonFile(const std::string& path, const JsonObject& obj);
