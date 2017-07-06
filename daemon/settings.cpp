#include "config.h"
#include "settings.h"
#include "daemon.h"
#include "path.h"
#include "logger.h"

#include <jsonrpc-lean/jsonreader.h>
#include <jsonrpc-lean/jsonwriter.h>


Config g_config;
Account g_account;
Settings g_settings;


bool Config::ReadFromDisk()
{
	return NativeJsonObject::ReadFromDisk(GetFile(ConfigFile), "config");
}
bool Config::WriteToDisk() const
{
	return NativeJsonObject::WriteToDisk(GetFile(ConfigFile, EnsureExists), "config");
}

bool Account::ReadFromDisk()
{
	return NativeJsonObject::ReadFromDisk(GetFile(AccountFile), "account");
}
bool Account::WriteToDisk() const
{
	return NativeJsonObject::WriteToDisk(GetFile(AccountFile, EnsureExists), "account");
}

bool Settings::ReadFromDisk()
{
	return NativeJsonObject::ReadFromDisk(GetFile(SettingsFile));
}
bool Settings::WriteToDisk() const
{
	return NativeJsonObject::WriteToDisk(GetFile(SettingsFile, EnsureExists));
}



static const JsonArray g_certificate_authorities {
	(JsonValue) JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIFjzCCA3egAwIBAgIJAJJQJtmVh2qbMA0GCSqGSIb3DQEBCwUAMF4xCzAJBgNV",
		"BAYTAklTMSIwIAYDVQQKDBlDeXBoZXJwdW5rIFBhcnRuZXJzLCBTbGYuMSswKQYD",
		"VQQDDCJDeXBoZXJwdW5rIFByaXZhY3kgTmV0d29yayBSb290IENBMB4XDTE3MDcw",
		"NTA5MDQyMFoXDTM3MDYzMDA5MDQyMFowXjELMAkGA1UEBhMCSVMxIjAgBgNVBAoM",
		"GUN5cGhlcnB1bmsgUGFydG5lcnMsIFNsZi4xKzApBgNVBAMMIkN5cGhlcnB1bmsg",
		"UHJpdmFjeSBOZXR3b3JrIFJvb3QgQ0EwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAw",
		"ggIKAoICAQC34jwxHqT78T76ctQlrchnq4wop5Fe8PlIr9gLp3TfzfpmdduEcKGm",
		"ggeC6O152HokpmPhwNftPlWcDQGq5sXJRm+don1TU1ChY0aMHVJr8M2TON/sGIXD",
		"vQeDWs8hHdzOz+62Dtf5EiXa6dTju1YUiKqur52KyXKdYIQlmftKJs8vbFE0iXLN",
		"9y9dmaqUS/HwgDGmq5VHgkrv2k5bNAKQej/u10C8yFI7N6sxe/dQkvMqtTda9dFJ",
		"ZoLCMvX+XLjo0j3mwd//EMTI9tAPn8olapWjU5BE/L7W90+YGMbnboOLogpHKZaR",
		"x0PBb7QUvqy2VaYg3kt/7pubR3bX61B4OdNtYGy9iec3Y2FkDXpF5oCTCjWrYCdU",
		"29yP6WVlNEFWUOqfyrZXrFeS/mAK6im6m0CyhCJj2/St0ShFknhQwXRlVdjLR+fM",
		"XHwKjtq6/I728ht+F2HGRwiF98zed+SkMU1c+v/dDhmFYIvEwfNAoQHhkBAZVLqw",
		"BcIc0kMtEnU0vlwChmBf/B+pbshOaP9bk4Zfawwq3XL3awW6VRGerJonBuPsSLZj",
		"8EjNw7EoyT+XEdV+Ia742KiH4ZylcXC3W/1jzeuqpZz4KxjnZ3olzxaUoXksmUZL",
		"NJX7S7zNq+jugXqpuVBlw85qAbIhwv02wSboRqlEL/15vhlkAnLrnQIDAQABo1Aw",
		"TjAdBgNVHQ4EFgQUrXEb2rrEK6p7ZJUyeit2i9qGfYwwHwYDVR0jBBgwFoAUrXEb",
		"2rrEK6p7ZJUyeit2i9qGfYwwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOC",
		"AgEAG9sbEMNKlkZB60K3MKTxDoF0JTJyhCGwbJJLiYSWuCuOzhdIxTJ/TP4yCHoc",
		"EYaSymf7hbcBLdXLTHUUHHK8Sy+l6D6LyQamDi+v0dTbdiQBBSNmo4CBnNCA2Xyb",
		"eHFpdGCK4IkGRd72vZ4dwIAF+LThqCND1hmbC8FxNW7lI8GWJBHw1hUJ465OOCB8",
		"vJmY03iMIUh9loVWyGl3asgbLZAwU/rEMWwDpPkIGQ0/ZISWfAvhkguDKg8fJ+oZ",
		"mztijuvnRK2A01ecxQ6TEsPdTxjKWq5T3ZSLAPsY0WaEToYgHta6Oc+6dDk5aCYv",
		"CpHDq8FSkKzeD+RLiXWH1A6MxCCoB1DxZHbSUO+eeYVQ+musdkOtZXBnL3DNA2Ut",
		"ZsyC6fDirc6mvmgshzkedlOFM5QJHBJ+QG2+vOp1Wlj6XwN6D4zuZ8QXbH/lh/wx",
		"cx+0oGsTzjIPuVG5SmysA3p9TuOOp69waAIJTfPcOktYdDn6OVNmXR9AWCsKVr6j",
		"MonCHl9WafOfEqNr2jhh5Lx/WpChik2ZFjGZ+HKDI1cR+NFI2WLAf1Og+lMUtViA",
		"jRPhEkBtvyGqJYx6WBpRN9p54kqoQ61dtK99jqSz7XDftWG9z7mv0T5t9l53ZpaM",
		"ZyIks1WsYohzxwejx/c91yAbKdQf6PYmBT/qoD8tHGtWAks=",
		"-----END CERTIFICATE-----",
	},
};

const JsonArray& Config::certificateAuthorities() const
{
	return g_certificate_authorities;
}


void Settings::Reset(bool deleteAllValues)
{
	if (deleteAllValues)
	{
		for (auto it = begin(); it != end(); )
		{
			if (_fields.find(it->first) == _fields.end())
				it = erase(it);
			else
				++it;
		}
	}
	for (const auto& p : _fields)
	{
		if (p.first != "location")
			p.second.reset();
	}
}
