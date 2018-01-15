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



static const JsonObject g_certificate_authorities {
	{ "ECDSA-256k1", (JsonValue) JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIEHTCCA8KgAwIBAgIJALJnZBxsuxbRMAoGCCqGSM49BAMEMIHoMQswCQYDVQQG",
		"EwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoT",
		"F1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVy",
		"bmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAe",
		"BgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBz",
		"ZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbTAeFw0xNDA0MTcxNzIxMzRa",
		"Fw0zNDA0MTIxNzIxMzRaMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0ExEzAR",
		"BgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNj",
		"ZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UEAxMX",
		"UHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJu",
		"ZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVybmV0",
		"YWNjZXNzLmNvbTBWMBAGByqGSM49AgEGBSuBBAAKA0IABB7ROYGdprNfDX0CHR7F",
		"IuMR8Sv3CVXyTJFqGIM6GvaS8HEHvLXsLRMbEMdiMvqLE+RCMNI82wNKRA61P2Ui",
		"u+SjggFUMIIBUDAdBgNVHQ4EFgQUmTwsDwUw8uyHViBcXsFlH9uWBBwwggEfBgNV",
		"HSMEggEWMIIBEoAUmTwsDwUw8uyHViBcXsFlH9uWBByhge6kgeswgegxCzAJBgNV",
		"BAYTAlVTMQswCQYDVQQIEwJDQTETMBEGA1UEBxMKTG9zQW5nZWxlczEgMB4GA1UE",
		"ChMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBAsTF1ByaXZhdGUgSW50",
		"ZXJuZXQgQWNjZXNzMSAwHgYDVQQDExdQcml2YXRlIEludGVybmV0IEFjY2VzczEg",
		"MB4GA1UEKRMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxLzAtBgkqhkiG9w0BCQEW",
		"IHNlY3VyZUBwcml2YXRlaW50ZXJuZXRhY2Nlc3MuY29tggkAsmdkHGy7FtEwDAYD",
		"VR0TBAUwAwEB/zAKBggqhkjOPQQDBANJADBGAiEA6WWy4+Td9LJ3HNYKzqfvMwvB",
		"Qeq8/d6uWFdJ0gi17DACIQCysjd6+CBR5YcTHxeSkF7IvvbVTO2axvXhbv8fIsQx",
		"Qw==",
		"-----END CERTIFICATE-----",
	} },
	{ "ECDSA-256r1", (JsonValue)JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIEHjCCA8WgAwIBAgIJANBplv/w3alWMAoGCCqGSM49BAMEMIHoMQswCQYDVQQG",
		"EwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoT",
		"F1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVy",
		"bmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAe",
		"BgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBz",
		"ZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbTAeFw0xNDA0MTcxNzMwMjNa",
		"Fw0zNDA0MTIxNzMwMjNaMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0ExEzAR",
		"BgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNj",
		"ZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UEAxMX",
		"UHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJu",
		"ZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVybmV0",
		"YWNjZXNzLmNvbTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL+pqsojiu0lBbw+",
		"2KpfDWAFUmyLh5ImRPxrJ76lgeY/i0g/q/jyiWK/lQsRXLiG78GO+gWf/k9/WBSb",
		"1SwJ7KSjggFUMIIBUDAdBgNVHQ4EFgQUP3QXfywjHCGBH55HVh5EFNHBll4wggEf",
		"BgNVHSMEggEWMIIBEoAUP3QXfywjHCGBH55HVh5EFNHBll6hge6kgeswgegxCzAJ",
		"BgNVBAYTAlVTMQswCQYDVQQIEwJDQTETMBEGA1UEBxMKTG9zQW5nZWxlczEgMB4G",
		"A1UEChMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBAsTF1ByaXZhdGUg",
		"SW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQDExdQcml2YXRlIEludGVybmV0IEFjY2Vz",
		"czEgMB4GA1UEKRMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxLzAtBgkqhkiG9w0B",
		"CQEWIHNlY3VyZUBwcml2YXRlaW50ZXJuZXRhY2Nlc3MuY29tggkA0GmW//DdqVYw",
		"DAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDBANHADBEAiBEAKGWcMmUawABsNg6l5bY",
		"Mt/nr2amk53mHfIrE4gkMAIgWzZRIJ4XzcXy0i4crrPrMIx8CYP8EQfvLI4rsVPg",
		"RP8=",
		"-----END CERTIFICATE-----",
	} },
	{ "ECDSA-521", (JsonValue)JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIEpzCCBAigAwIBAgIJAKbEcZk5BSQwMAoGCCqGSM49BAMEMIHoMQswCQYDVQQG",
		"EwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoT",
		"F1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVy",
		"bmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAe",
		"BgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBz",
		"ZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbTAeFw0xNDA0MTcxNzMxMTda",
		"Fw0zNDA0MTIxNzMxMTdaMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0ExEzAR",
		"BgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNj",
		"ZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UEAxMX",
		"UHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJu",
		"ZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVybmV0",
		"YWNjZXNzLmNvbTCBmzAQBgcqhkjOPQIBBgUrgQQAIwOBhgAEAZpxAANwoBkstZQb",
		"5xGN4qodr52/gWoRTQD5LJePOE9+WdkPmmxf5MD6Ov+VDhZLi2RZ18ACbZ5BDVGF",
		"lt+l76djARDd8ROgrcXUW+zA5LqBSwuV3QeBk3fLcHIhB9GO0NE7DJVTQwnQ+FNa",
		"v16VL+0etX3D1g+ayC5AsH6rxgEFsSafo4IBVDCCAVAwHQYDVR0OBBYEFPisdXMF",
		"uwSE8d6sh50WCAF8yLmuMIIBHwYDVR0jBIIBFjCCARKAFPisdXMFuwSE8d6sh50W",
		"CAF8yLmuoYHupIHrMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0ExEzARBgNV",
		"BAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNz",
		"MSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UEAxMXUHJp",
		"dmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQg",
		"QWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVybmV0YWNj",
		"ZXNzLmNvbYIJAKbEcZk5BSQwMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwQDgYwA",
		"MIGIAkIAkeK3axSbFAXKDKcI72eVT55JYZpRV4PYDgbQNSUGsyzT0iBQaiEP15EU",
		"HT4Stz531yJ3j3gm6JuWqDpqmMX4dToCQgH83DbGDvpx97wJtG1i+yg9GXhzmyYM",
		"4RCsSuLgT98WTwZXnoPUyh/Qbgiihnjkg/F6v7vvMi8P6AbTKwixmCyZtA==",
		"-----END CERTIFICATE-----",
	} },
	{ "RSA-2048", (JsonValue)JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIFqzCCBJOgAwIBAgIJAKZ7D5Yv87qDMA0GCSqGSIb3DQEBDQUAMIHoMQswCQYD",
		"VQQGEwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNV",
		"BAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIElu",
		"dGVybmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3Mx",
		"IDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkB",
		"FiBzZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbTAeFw0xNDA0MTcxNzM1",
		"MThaFw0zNDA0MTIxNzM1MThaMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0Ex",
		"EzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQg",
		"QWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UE",
		"AxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50",
		"ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVy",
		"bmV0YWNjZXNzLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAPXD",
		"L1L9tX6DGf36liA7UBTy5I869z0UVo3lImfOs/GSiFKPtInlesP65577nd7UNzzX",
		"lH/P/CnFPdBWlLp5ze3HRBCc/Avgr5CdMRkEsySL5GHBZsx6w2cayQ2EcRhVTwWp",
		"cdldeNO+pPr9rIgPrtXqT4SWViTQRBeGM8CDxAyTopTsobjSiYZCF9Ta1gunl0G/",
		"8Vfp+SXfYCC+ZzWvP+L1pFhPRqzQQ8k+wMZIovObK1s+nlwPaLyayzw9a8sUnvWB",
		"/5rGPdIYnQWPgoNlLN9HpSmsAcw2z8DXI9pIxbr74cb3/HSfuYGOLkRqrOk6h4RC",
		"OfuWoTrZup1uEOn+fw8CAwEAAaOCAVQwggFQMB0GA1UdDgQWBBQv63nQ/pJAt5tL",
		"y8VJcbHe22ZOsjCCAR8GA1UdIwSCARYwggESgBQv63nQ/pJAt5tLy8VJcbHe22ZO",
		"sqGB7qSB6zCB6DELMAkGA1UEBhMCVVMxCzAJBgNVBAgTAkNBMRMwEQYDVQQHEwpM",
		"b3NBbmdlbGVzMSAwHgYDVQQKExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4G",
		"A1UECxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBAMTF1ByaXZhdGUg",
		"SW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQpExdQcml2YXRlIEludGVybmV0IEFjY2Vz",
		"czEvMC0GCSqGSIb3DQEJARYgc2VjdXJlQHByaXZhdGVpbnRlcm5ldGFjY2Vzcy5j",
		"b22CCQCmew+WL/O6gzAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBDQUAA4IBAQAn",
		"a5PgrtxfwTumD4+3/SYvwoD66cB8IcK//h1mCzAduU8KgUXocLx7QgJWo9lnZ8xU",
		"ryXvWab2usg4fqk7FPi00bED4f4qVQFVfGfPZIH9QQ7/48bPM9RyfzImZWUCenK3",
		"7pdw4Bvgoys2rHLHbGen7f28knT2j/cbMxd78tQc20TIObGjo8+ISTRclSTRBtyC",
		"GohseKYpTS9himFERpUgNtefvYHbn70mIOzfOJFTVqfrptf9jXa9N8Mpy3ayfodz",
		"1wiqdteqFXkTYoSDctgKMiZ6GdocK9nMroQipIQtpnwd4yBDWIyC6Bvlkrq5TQUt",
		"YDQ8z9v+DMO6iwyIDRiU",
		"-----END CERTIFICATE-----",
	} },
	{ "RSA-3072", (JsonValue)JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIGqzCCBROgAwIBAgIJAL2eMgp0qeyCMA0GCSqGSIb3DQEBDQUAMIHoMQswCQYD",
		"VQQGEwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNV",
		"BAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIElu",
		"dGVybmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3Mx",
		"IDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkB",
		"FiBzZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbTAeFw0xNDA0MTcxNzM5",
		"MDZaFw0zNDA0MTIxNzM5MDZaMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0Ex",
		"EzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQg",
		"QWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UE",
		"AxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50",
		"ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVy",
		"bmV0YWNjZXNzLmNvbTCCAaIwDQYJKoZIhvcNAQEBBQADggGPADCCAYoCggGBAN+H",
		"UO8UiS16NJl9PY1W3nuRU5+i1eay4aF3anplKB/9jeH9dy1dzMVu2vwfndOpdoAa",
		"RCZde0dGK8piD48zCTO4EJuSDvMD/P+YVzagernhM0t0tSYPmTcEI04h9KDbzFaB",
		"iYQz+wMrb52QyN+cnTm02K/01UeBwifKHqBe1GTV0GGECOTyzRDxUZ5Ro2x2XDhW",
		"/1wxn4olLxpV+17I5Of6SXVCP4aD8qem81ryXSmp1WVlBMPRXS74v2aKMRJoaxV+",
		"NB5O6lcND6mVLgie1WA3/LFgjY08bRFanfoX4AOj4TQvOHgxrtKKoCueMgQP/Ey1",
		"7hcCY0cnyyMYZvDT4zR7Rl29wQGV4cPLzC0fyQ/ry4UjzTtMdaIb3zB4iErYMV+d",
		"TqBh3VzkhjYPf/i0YJHGOH1Z3jcNXuvL14H238DF6eKcWZzZzSHg7aQZtjwehTVD",
		"Z5Rt/iqxD5XlmzrpgbHy6jwiLtZZ6tBqckRjVcBzsQnYcA1r3WPTzwjlOBI2mwID",
		"AQABo4IBVDCCAVAwHQYDVR0OBBYEFP17KSHfvxCi6I9MddTzo5YnXU3NMIIBHwYD",
		"VR0jBIIBFjCCARKAFP17KSHfvxCi6I9MddTzo5YnXU3NoYHupIHrMIHoMQswCQYD",
		"VQQGEwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNV",
		"BAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIElu",
		"dGVybmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3Mx",
		"IDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkB",
		"FiBzZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbYIJAL2eMgp0qeyCMAwG",
		"A1UdEwQFMAMBAf8wDQYJKoZIhvcNAQENBQADggGBANXsaYsnegzIcEMQdOMMCuj7",
		"GHrp5XIyvxQ7H9fp57SPi3dFWZEKcd8N6VdjDpBNHYIwyUc3kSPnnfA03AeKOKDs",
		"sJNV/ebyRTouzZrUgiwFshi0FIralfLPaJouYUaqm7p2uji/m672nDLzy1HsBNNs",
		"HPGJQ5ahXvCbXQSKbRxMIT1AX69iD/+dW/aPjFNikfdtkvg6xuVBgOFmttkC1CL8",
		"+L+UlQfHlEWq7/USN8Ob6u2TbU2LfPeWtaBJFKLHsejIfGzkWQMp4m/r0h6cain9",
		"UNhe9IN19GGSm9W1YkZwANmpPL2AZgGYpLu8jFrz2n+zR7cyNENn2j69h1+tiPIz",
		"s4SrEKNBvZDxT6Z7F5z6BHOB/7l+68TbBLiNb2Ahn0pcbqAaY00dYgVGCRY/g6gV",
		"ceaLeH73I0HclfCeMUW/FQpxL83UM8QcOoSe+Z+3YQc87/z7KESTLzcVuB/caZjY",
		"00uYIdq+89LCaymtg4kEe805OO6y9vEqaIEqzpvwUw==",
		"-----END CERTIFICATE-----",
	} },
	{ "RSA-4096", (JsonValue)JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIIHqzCCBZOgAwIBAgIJAJ0u+vODZJntMA0GCSqGSIb3DQEBDQUAMIHoMQswCQYD",
		"VQQGEwJVUzELMAkGA1UECBMCQ0ExEzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNV",
		"BAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIElu",
		"dGVybmV0IEFjY2VzczEgMB4GA1UEAxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3Mx",
		"IDAeBgNVBCkTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkB",
		"FiBzZWN1cmVAcHJpdmF0ZWludGVybmV0YWNjZXNzLmNvbTAeFw0xNDA0MTcxNzQw",
		"MzNaFw0zNDA0MTIxNzQwMzNaMIHoMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0Ex",
		"EzARBgNVBAcTCkxvc0FuZ2VsZXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQg",
		"QWNjZXNzMSAwHgYDVQQLExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UE",
		"AxMXUHJpdmF0ZSBJbnRlcm5ldCBBY2Nlc3MxIDAeBgNVBCkTF1ByaXZhdGUgSW50",
		"ZXJuZXQgQWNjZXNzMS8wLQYJKoZIhvcNAQkBFiBzZWN1cmVAcHJpdmF0ZWludGVy",
		"bmV0YWNjZXNzLmNvbTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBALVk",
		"hjumaqBbL8aSgj6xbX1QPTfTd1qHsAZd2B97m8Vw31c/2yQgZNf5qZY0+jOIHULN",
		"De4R9TIvyBEbvnAg/OkPw8n/+ScgYOeH876VUXzjLDBnDb8DLr/+w9oVsuDeFJ9K",
		"V2UFM1OYX0SnkHnrYAN2QLF98ESK4NCSU01h5zkcgmQ+qKSfA9Ny0/UpsKPBFqsQ",
		"25NvjDWFhCpeqCHKUJ4Be27CDbSl7lAkBuHMPHJs8f8xPgAbHRXZOxVCpayZ2SND",
		"fCwsnGWpWFoMGvdMbygngCn6jA/W1VSFOlRlfLuuGe7QFfDwA0jaLCxuWt/BgZyl",
		"p7tAzYKR8lnWmtUCPm4+BtjyVDYtDCiGBD9Z4P13RFWvJHw5aapx/5W/CuvVyI7p",
		"Kwvc2IT+KPxCUhH1XI8ca5RN3C9NoPJJf6qpg4g0rJH3aaWkoMRrYvQ+5PXXYUzj",
		"tRHImghRGd/ydERYoAZXuGSbPkm9Y/p2X8unLcW+F0xpJD98+ZI+tzSsI99Zs5wi",
		"jSUGYr9/j18KHFTMQ8n+1jauc5bCCegN27dPeKXNSZ5riXFL2XX6BkY68y58UaNz",
		"meGMiUL9BOV1iV+PMb7B7PYs7oFLjAhh0EdyvfHkrh/ZV9BEhtFa7yXp8XR0J6vz",
		"1YV9R6DYJmLjOEbhU8N0gc3tZm4Qz39lIIG6w3FDAgMBAAGjggFUMIIBUDAdBgNV",
		"HQ4EFgQUrsRtyWJftjpdRM0+925Y6Cl08SUwggEfBgNVHSMEggEWMIIBEoAUrsRt",
		"yWJftjpdRM0+925Y6Cl08SWhge6kgeswgegxCzAJBgNVBAYTAlVTMQswCQYDVQQI",
		"EwJDQTETMBEGA1UEBxMKTG9zQW5nZWxlczEgMB4GA1UEChMXUHJpdmF0ZSBJbnRl",
		"cm5ldCBBY2Nlc3MxIDAeBgNVBAsTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSAw",
		"HgYDVQQDExdQcml2YXRlIEludGVybmV0IEFjY2VzczEgMB4GA1UEKRMXUHJpdmF0",
		"ZSBJbnRlcm5ldCBBY2Nlc3MxLzAtBgkqhkiG9w0BCQEWIHNlY3VyZUBwcml2YXRl",
		"aW50ZXJuZXRhY2Nlc3MuY29tggkAnS7684Nkme0wDAYDVR0TBAUwAwEB/zANBgkq",
		"hkiG9w0BAQ0FAAOCAgEAJsfhsPk3r8kLXLxY+v+vHzbr4ufNtqnL9/1Uuf8NrsCt",
		"pXAoyZ0YqfbkWx3NHTZ7OE9ZRhdMP/RqHQE1p4N4Sa1nZKhTKasV6KhHDqSCt/dv",
		"Em89xWm2MVA7nyzQxVlHa9AkcBaemcXEiyT19XdpiXOP4Vhs+J1R5m8zQOxZlV1G",
		"tF9vsXmJqWZpOVPmZ8f35BCsYPvv4yMewnrtAC8PFEK/bOPeYcKN50bol22QYaZu",
		"LfpkHfNiFTnfMh8sl/ablPyNY7DUNiP5DRcMdIwmfGQxR5WEQoHL3yPJ42LkB5zs",
		"6jIm26DGNXfwura/mi105+ENH1CaROtRYwkiHb08U6qLXXJz80mWJkT90nr8Asj3",
		"5xN2cUppg74nG3YVav/38P48T56hG1NHbYF5uOCske19F6wi9maUoto/3vEr0rnX",
		"JUp2KODmKdvBI7co245lHBABWikk8VfejQSlCtDBXn644ZMtAdoxKNfR2WTFVEwJ",
		"iyd1Fzx0yujuiXDROLhISLQDRjVVAvawrAtLZWYK31bY7KlezPlQnl/D9Asxe85l",
		"8jO5+0LdJ6VyOs/Hd4w52alDW/MFySDZSfQHMTIc30hLBJ8OnCEIvluVQQ2UQvoW",
		"+no177N9L2Y+M9TcTA62ZyMXShHQGeh20rb4kK8f+iFX8NxtdHVSkxMEFSfDDyQ=",
		"-----END CERTIFICATE-----",
	} },
	{ "default", (JsonValue)JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"MIID2jCCA0OgAwIBAgIJAOtqMkR2JSXrMA0GCSqGSIb3DQEBBQUAMIGlMQswCQYD",
		"VQQGEwJVUzELMAkGA1UECBMCT0gxETAPBgNVBAcTCENvbHVtYnVzMSAwHgYDVQQK",
		"ExdQcml2YXRlIEludGVybmV0IEFjY2VzczEjMCEGA1UEAxMaUHJpdmF0ZSBJbnRl",
		"cm5ldCBBY2Nlc3MgQ0ExLzAtBgkqhkiG9w0BCQEWIHNlY3VyZUBwcml2YXRlaW50",
		"ZXJuZXRhY2Nlc3MuY29tMB4XDTEwMDgyMTE4MjU1NFoXDTIwMDgxODE4MjU1NFow",
		"gaUxCzAJBgNVBAYTAlVTMQswCQYDVQQIEwJPSDERMA8GA1UEBxMIQ29sdW1idXMx",
		"IDAeBgNVBAoTF1ByaXZhdGUgSW50ZXJuZXQgQWNjZXNzMSMwIQYDVQQDExpQcml2",
		"YXRlIEludGVybmV0IEFjY2VzcyBDQTEvMC0GCSqGSIb3DQEJARYgc2VjdXJlQHBy",
		"aXZhdGVpbnRlcm5ldGFjY2Vzcy5jb20wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJ",
		"AoGBAOlVlkHcxfN5HAswpryG7AN9CvcvVzcXvSEo91qAl/IE8H0knKZkIAhe/z3m",
		"hz0t91dBHh5yfqwrXlGiyilplVB9tfZohvcikGF3G6FFC9j40GKP0/d22JfR2vJt",
		"4/5JKRBlQc9wllswHZGmPVidQbU0YgoZl00bAySvkX/u1005AgMBAAGjggEOMIIB",
		"CjAdBgNVHQ4EFgQUl8qwY2t+GN0pa/wfq+YODsxgVQkwgdoGA1UdIwSB0jCBz4AU",
		"l8qwY2t+GN0pa/wfq+YODsxgVQmhgaukgagwgaUxCzAJBgNVBAYTAlVTMQswCQYD",
		"VQQIEwJPSDERMA8GA1UEBxMIQ29sdW1idXMxIDAeBgNVBAoTF1ByaXZhdGUgSW50",
		"ZXJuZXQgQWNjZXNzMSMwIQYDVQQDExpQcml2YXRlIEludGVybmV0IEFjY2VzcyBD",
		"QTEvMC0GCSqGSIb3DQEJARYgc2VjdXJlQHByaXZhdGVpbnRlcm5ldGFjY2Vzcy5j",
		"b22CCQDrajJEdiUl6zAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUAA4GBAByH",
		"atXgZzjFO6qctQWwV31P4qLelZzYndoZ7olY8ANPxl7jlP3YmbE1RzSnWtID9Gge",
		"fsKHi1jAS9tNP2E+DCZiWcM/5Y7/XKS/6KvrPQT90nM5klK9LfNvS+kFabMmMBe2",
		"llQlzAzFiIfabACTQn84QLeLOActKhK8hFJy2Gy6",
		"-----END CERTIFICATE-----",
	} },
};

const JsonArray& Config::certificateAuthority(const std::string& type) const
{
	return g_certificate_authorities.at(type).AsArray();
}
const JsonObject& Config::certificateAuthorities() const
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
