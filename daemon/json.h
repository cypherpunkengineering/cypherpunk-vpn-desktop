#pragma once

#include <string>
#include <jsonrpc-lean/value.h>

typedef jsonrpc::Value JsonValue;
typedef jsonrpc::Value::Struct JsonObject;
typedef jsonrpc::Value::Array JsonArray;

JsonObject ReadJsonFile(const std::string& path);
void WriteJsonFile(const std::string& path, const JsonObject& obj);
