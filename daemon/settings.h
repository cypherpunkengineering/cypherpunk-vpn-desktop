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

	const JsonArray& certificateAuthorities() const;
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

	JsonField(std::string, remotePort, "udp:7133")
	JsonField(std::string, location, "tokyodev") // FIXME: don't hardcode
	JsonField(int, localPort, 0)
	JsonField(int, mtu, 1500)
	JsonField(std::string, encryption, "default") // "none", "default", "strong" or "stealth"

	JsonField(std::string, firewall, "off")
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
