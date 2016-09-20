#include "config.h"
#include "settings.h"
#include "daemon.h"
#include "path.h"
#include "logger.h"

#include <jsonrpc-lean/jsonreader.h>
#include <jsonrpc-lean/jsonwriter.h>


Settings g_settings;


namespace jsonrpc {
	bool operator ==(const jsonrpc::Value& lhs, const jsonrpc::Value& rhs)
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
}

Settings::Settings()
{

}

JsonValue& Settings::operator[](const std::string& name)
{
	return JsonObject::at(name);
}

void Settings::ReadFromDisk()
{
	try
	{
		map() = ReadJsonFile(GetPath(SettingsFile));
	}
	catch (const std::system_error& e)
	{
		LOG(WARNING) << "Couldn't open settings file: " << e;
	}
	catch (const std::exception& e)
	{
		LOG(ERROR) << "Invalid settings file: " << e;
	}
}

void Settings::WriteToDisk()
{
	WriteJsonFile(GetPath(SettingsFile), map());
}

void Settings::OnChanged(const std::vector<std::string>& params) noexcept
{
	try
	{
		WriteToDisk();
	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << "Failed to write settings: " << e;
	}
	if (g_daemon) g_daemon->OnSettingsChanged(params);
}
