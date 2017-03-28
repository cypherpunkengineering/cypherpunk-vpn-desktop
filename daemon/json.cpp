#include "config.h"

#if OS_LINUX
#include <string.h>
#endif

#include "json.h"
#include "logger.h"
#include "path.h"

#include <jsonrpc-lean/jsonreader.h>
#include <jsonrpc-lean/jsonwriter.h>

std::string ToJson(const JsonValue& value)
{
	JsonWriter writer;
	value.Write(writer);
	auto data = writer.GetData();
	return std::string(data->GetData(), data->GetSize());
}

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


bool NativeJsonObject::ReadFromDisk(const std::string& path, const char* type)
{
	try
	{
		for (auto& p : ReadJsonFile(path))
		{
			try
			{
				JsonObject::operator[](p.first) = std::move(p.second);
				if (_on_changed) _on_changed(p.first.c_str());
			}
			catch (std::exception&)
			{
				LOG(WARNING) << "Incorrect data type " << (int)p.second.GetType() << " for " << type << " item " << p.first << ", ignoring";
			}
		}
		return true;
	}
	catch (const std::system_error& e)
	{
		LOG(WARNING) << "Couldn't open " << type << " file: " << e;
		return false;
	}
	catch (const std::exception& e)
	{
		LOG(ERROR) << "Invalid " << type << " file: " << e;
		return false;
	}
}

bool NativeJsonObject::WriteToDisk(const std::string& path, const char* type) const
{
	try
	{
		WriteJsonFile(path, *this);
		return true;
	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << "Failed to write " << type << " file: " << e;
		return false;
	}
}

void NativeJsonObject::RemoveUnknownFields()
{
	for (auto it = JsonObject::begin(); it != JsonObject::end(); )
	{
		if (_fields.find(it->first) == _fields.end())
			it = erase(it);
		else
			++it;
	}
}
