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
	JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIICPzCCAcagAwIBAgIJAMLqTfX9NxfOMAoGCCqGSM49BAMCMF4xCzAJBgNVBAYT",
		"AklTMSIwIAYDVQQKDBlDeXBoZXJwdW5rIFBhcnRuZXJzLCBTbGYuMSswKQYDVQQD",
		"DCJDeXBoZXJwdW5rIFByaXZhY3kgTmV0d29yayBSb290IENBMB4XDTE3MDcwNDEw",
		"MjQ0M1oXDTM3MDYyOTEwMjQ0M1owXjELMAkGA1UEBhMCSVMxIjAgBgNVBAoMGUN5",
		"cGhlcnB1bmsgUGFydG5lcnMsIFNsZi4xKzApBgNVBAMMIkN5cGhlcnB1bmsgUHJp",
		"dmFjeSBOZXR3b3JrIFJvb3QgQ0EwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAARrlCaM",
		"MANPH64vDoI+MdnebGKoeik6rvPiocsfhd0Sup7VLsCeVsGy9ix++fwZbSvyIuhW",
		"FUcYKjrIBUgdAAoTes27O8HFcKvqh9KhsJR3N5EZOJeW9hBrSJPUzQD04m2jUDBO",
		"MB0GA1UdDgQWBBSRIb30U/AwnK9dFL90/5PfBoG/YDAfBgNVHSMEGDAWgBSRIb30",
		"U/AwnK9dFL90/5PfBoG/YDAMBgNVHRMEBTADAQH/MAoGCCqGSM49BAMCA2cAMGQC",
		"MHyZTGMvj72Cr6QI/O1GygRcB+eJ0h+995tYolp3opomfB7okVCep+VOcAGfwgA5",
		"bAIwf9rCYJmZXbJdIh66Y3ANpuD0zy6vVSTF4l/MLVkivqbd08u4kutNR/OTkAy9",
		"OPgc",
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
