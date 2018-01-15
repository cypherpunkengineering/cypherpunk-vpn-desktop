#pragma once

#include "json.h"

class Config : public NativeJsonObject
{
public:
	bool ReadFromDisk();
	bool WriteToDisk() const;

	JsonField(JsonObject, regions, {});
	JsonField(JsonObject, locations, {});
	JsonField(JsonObject, countryNames, {});
	JsonField(JsonObject, regionNames, {});
	JsonField(JsonArray, regionOrder, {});

	const JsonArray& certificateAuthority(const std::string& type) const;
	const JsonObject& certificateAuthorities() const;
};
extern Config g_config;


class Account : public NativeJsonObject
{
public:
	bool ReadFromDisk();
	bool WriteToDisk() const;

	JsonField(JsonObject, privacy, {});

	void Reset() { RemoveUnknownFields(); }
};
extern Account g_account;


class Settings : public NativeJsonObject
{
public:
	bool ReadFromDisk();
	bool WriteToDisk() const;
	void Reset(bool deleteAllValues = true);

	JsonField(std::string, protocol, "udp")
	JsonField(int, remotePort, 0) // 0 (auto), 53, 1194, 8080, 9021
	JsonField(std::string, location, "us_atlanta") // FIXME: don't hardcode
	JsonField(std::string, locationFlag, "") // "cypherplay", "fastest", "fastest-us", "fastest-uk" or empty
	JsonField(std::string, fastest, "") // a cached fastest server which the daemon may switch to when (re)connecting if locationFlag is being used
	JsonField(int, localPort, 0)
	JsonField(int, mtu, 1500)
	JsonField(std::string, cipher, "AES-128-GCM")
	JsonField(std::string, auth, "none")
	JsonField(std::string, serverCertificate, "RSA-2048")

	JsonField(std::string, firewall, "auto")
	JsonField(bool, allowLAN, true)
	JsonField(bool, blockIPv6, true)
	JsonField(bool, overrideDNS, true)
	JsonField(bool, optimizeDNS, false) // only usable when overrideDNS == true
	JsonField(bool, blockAds, false) // only usable when overrideDNS == true
	JsonField(bool, blockMalware, true) // only usable when overrideDNS == true
	JsonField(bool, routeDefault, true) // add a default (or 0/1+1/1) route to the VPN interface
#if OS_OSX
	JsonField(bool, exemptApple, false)
#endif

	JsonField(bool, runOpenVPNAsRoot, true)

	// TODO: These are actually user-specific settings
	JsonField(bool, autoConnect, false)
	JsonField(bool, showNotifications, true)

	JsonField(JsonArray, favorites, {});
	JsonField(JsonArray, recent, {}); // deprecated
	JsonField(JsonObject, lastConnected, {});

	const JsonObject& currentLocation() const { return g_config.locations().at(location()).AsObject(); }
};
extern Settings g_settings;
