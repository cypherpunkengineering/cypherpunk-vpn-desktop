#include "config.h"
#include "json.h"
#include "path.h"

#include <jsonrpc-lean/jsonreader.h>
#include <jsonrpc-lean/jsonwriter.h>

JsonObject ReadJsonFile(const std::string& path)
{
	std::string text = ReadFile(GetPath(SettingsFile));
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
