#include "config.h"

#if OS_LINUX
#include <string.h>
#endif

#include "json.h"
#include "logger.h"
#include "path.h"

#include <jsonrpc-lean/jsonreader.h>
#include <jsonrpc-lean/jsonwriter.h>

JsonObject ReadJsonFile(const std::string& path)
{
	std::string text = ReadFile(path);
	jsonrpc::JsonReader reader(text);
	return std::move(const_cast<JsonObject&>(reader.GetValue().AsStruct()));
}

void WriteJsonFile(const std::string& path, const JsonObject& obj)
{
	jsonrpc::JsonWriter writer;
	writer.StartStruct();
	for (auto& element : obj) {
		writer.StartStructElement(element.first);
		element.second.Write(writer);
		writer.EndStructElement();
	}
	writer.EndStruct();
	auto data = writer.GetData();
	WriteFile(path, data->GetData(), data->GetSize());
}

namespace jsonrpc {
	bool operator ==(const JsonValue& lhs, const JsonValue& rhs)
	{
		auto type = lhs.GetType();
		if (type == rhs.GetType())
		{
			switch (type)
			{
			case jsonrpc::Value::Type::ARRAY:
				return lhs.AsArray() == rhs.AsArray();
			case jsonrpc::Value::Type::BINARY:
				return lhs.AsBinary() == rhs.AsBinary();
			case jsonrpc::Value::Type::BOOLEAN:
				return lhs.AsBoolean() == rhs.AsBoolean();
			case jsonrpc::Value::Type::DATE_TIME:
				return lhs.AsDateTime() == rhs.AsDateTime();
			case jsonrpc::Value::Type::DOUBLE:
				return lhs.AsDouble() == rhs.AsDouble();
			case jsonrpc::Value::Type::INTEGER_32:
				return lhs.AsInteger32() == rhs.AsInteger32();
			case jsonrpc::Value::Type::INTEGER_64:
				return lhs.AsInteger64() == rhs.AsInteger64();
			case jsonrpc::Value::Type::NIL:
				return true;
			case jsonrpc::Value::Type::STRING:
				return lhs.AsString() == rhs.AsString();
			case jsonrpc::Value::Type::STRUCT:
				return lhs.AsStruct() == rhs.AsStruct();
			}
		}
		return false;
	}
	bool operator !=(const JsonValue& lhs, const JsonValue& rhs)
	{
		return !(lhs == rhs);
	}
}






void NativeJsonObjectMapping::AddMember(MemberInfo member)
{
	std::string name = member.name;
	_members.emplace(std::pair<const std::string, MemberInfo>(std::move(name), std::move(member)));
}

const NativeJsonObjectMapping::MemberInfo& NativeJsonObjectMapping::GetMember(const std::string& name)
{
	return _members.at(name);
}

bool NativeJsonObjectMapping::SerializeMember(const std::string& name, JsonValue& target, const Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		target = JsonValue(it->second.serialize(context));
		return true;
	}
	return false;
}

bool NativeJsonObjectMapping::SerializeMember(const std::string& name, JsonObject& container, const Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		container[name] = JsonValue(it->second.serialize(context));
		return true;
	}
	return false;
}

bool NativeJsonObjectMapping::SerializeMember(const std::string& name, JsonWriter& writer, const Context* context)
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

//bool NativeJsonObjectMapping::DeserializeMember(const std::string& name, const JsonValue& value, Context* context)
//{
//
//}

bool NativeJsonObjectMapping::DeserializeMember(const std::string& name, JsonValue&& value, Context* context)
{
	auto it = _members.find(name);
	if (it != _members.end())
	{
		it->second.deserialize(std::move(value), context);
		return true;
	}
	return false;
}

void NativeJsonObjectMapping::SerializeAllMembers(JsonObject& container, const Context* context)
{

}

void NativeJsonObjectMapping::SerializeAllMembers(JsonWriter& writer, const Context* context)
{

}

void NativeJsonObjectMapping::DeserializeAllMembers(const JsonObject& source, const Context* context)
{

}

void NativeJsonObjectMapping::DeserializeAllMembersAndConsume(JsonObject& source, const Context* context)
{

}








/*

JsonValue NativeJsonObject::SerializeMember(std::string name)
{
	auto it = _members.find(name);
	if (it != _members.end())
		return it->serialize(this);
	else if (_keep_unknown)
	{
		auto it2 = _unknown.find(name);
		if (it2 != _unknown.end())
			return it2->second;
	}
	throw std::out_of_range_exception("member \"" + name + "\" not found");
}

void NativeJsonObject::DeserializeMember(std::string name, JsonValue&& value)
{
	auto it = _members.find(name);
	if (it != _members.end())
		it->deserialize(this, std::move(p.second));
	else if (_keep_unknown)
		_unknown[name] = std::move(p.second);
	if (_on_changed)
		_on_changed(name);
}

void NativeJsonObject::Read(JsonObject&& src)
{
	for (auto& p : src)
	{
		DeserializeMember(p.first, std::move(p.second));
	}
	src.clear();
}

void NativeJsonObject::Write(JsonObject& dst)
{

}

std::string NativeJsonObject::ToJson()
{
	std::string result;
	ToJson([&result](const char* data, size_t size) { result.assign(data, size); });
	return result;
}

void NativeJsonObject::ToJson(std::function<void(const char* data, size_t size)> cb)
{
	if (cb)
	{
		jsonrpc::JsonWriter writer;
		writer.StartStruct();
		for (auto& element : obj) {
			writer.StartStructElement(element.first);
			element.second.Write(writer);
			writer.EndStructElement();
		}
		writer.EndStruct();
		auto data = writer.GetData();
		cb(data->GetData(), data->GetSize());
	}
}
*/
