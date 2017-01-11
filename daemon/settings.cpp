#include "config.h"
#include "settings.h"
#include "daemon.h"
#include "path.h"
#include "logger.h"

#include <jsonrpc-lean/jsonreader.h>
#include <jsonrpc-lean/jsonwriter.h>


Settings g_settings;

Settings::Settings()
{

}

JsonValue& Settings::at(const std::string& name)
{
	return JsonObject::at(name);
}

JsonValue& Settings::operator[](const std::string& name)
{
	//return JsonObject::at(name);
	return JsonObject::operator[](name);
}

void Settings::ReadFromDisk()
{
	try
	{
		for (auto& p : ReadJsonFile(GetFile(SettingsFile)))
			JsonObject::operator[](p.first) = std::move(p.second);
		std::vector<std::string> names;
		for (auto& p : map())
			names.push_back(p.first);
		OnChanged(names);
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
	WriteJsonFile(GetFile(SettingsFile, EnsureExists), map());
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
