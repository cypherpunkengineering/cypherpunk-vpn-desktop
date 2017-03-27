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
		"MIIFiTCCA3GgAwIBAgICEAAwDQYJKoZIhvcNAQELBQAwUTELMAkGA1UEBhMCSVMx",
		"HDAaBgNVBAoME0N5cGhlcnB1bmsgUGFydG5lcnMxJDAiBgNVBAMMG0N5cGhlcnB1",
		"bmsgUGFydG5lcnMgUm9vdCBDQTAeFw0xNjA5MDYxNTI5MzBaFw0yNjA5MDQxNTI5",
		"MzBaMFkxCzAJBgNVBAYTAklTMRwwGgYDVQQKDBNDeXBoZXJwdW5rIFBhcnRuZXJz",
		"MSwwKgYDVQQDDCNDeXBoZXJwdW5rIFBhcnRuZXJzIEludGVybWVkaWF0ZSBDQTCC",
		"AiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBALVblDGKSLfQr8zk37Pmbce1",
		"nJ28hkIf5HvdFUIVY+396Qfjx9YNY3pTl/Bwjb0JwT7KHnHPkLtwdRgT74mPIK1j",
		"TDX4TjDMcWSUD9Bn2BppeHHj10zhEiMGZxlDkorR00FygM+pS6A9u9ack5PHzveY",
		"AOFwNh0SW20CQk+3/Ph+CcHbNeanfNt8U2UKBygyVkRTV3sYkIL6g7GQJ9th6YAJ",
		"mg2p3kU5ZadxslQaQBcM0G9kWBWsYif0IvAjh4rs1B0BHUPZpzsR062DkYHYJeSq",
		"cenfVfByXx9CW/tC/cDhIaD9dZxPschU4rVPShy6yM6B5WjKUfAGTKWdfDG2c/6S",
		"2iELvvRj2VFuBt5XVR39c7eIIvyGcfPrMPvYTQhP5+eGL92wsMqKoxosz4ZWiIHa",
		"Mb9cGjHupJRN1qpjnFN/fwTLm14JjaHklXLXF9ojCHbSWL3aXKX0lTuFOfY7A/zx",
		"hknbCijEQ3pxKLpJY3VjokMhlGrq+BYla+mKpeRKNJ7CgsM6MEO3yiO3n4CF0ZyS",
		"1DGrDAnrAPlA2bDX2LeFNPkt0A3Vv9BV6vgcahIcIRZjs5UVYN9XmErlESXgHm97",
		"Hb5QaYSgDBA4ekEE09dtH1CWKJREdtX38z3iN4pr7XXXlF0lM+aKr9rFeHB/MiWg",
		"PzJHBzmkhwcUYXhGLsVVAgMBAAGjYzBhMB0GA1UdDgQWBBRvC1oTePuUSlByx3pE",
		"MQnjTx5MUDAfBgNVHSMEGDAWgBTjkvrWu+Pe+eyx9dI35+jHACfjbTAPBgNVHRME",
		"CDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBhjANBgkqhkiG9w0BAQsFAAOCAgEANkiw",
		"o2Lsol6a0OnK52mmVgw2Al73Iak8NP+FGiTW+BFqxeBqiz9X9nI/03Z/keVla4Nx",
		"R0ziKh4sWjSa1ik9/XmjaRQ3c/BeDncwx7R51FmoVcdBMXwYUckVvtt0JOuT2yHP",
		"NekIZfiT+nBz9BPyxvpWZqocFBjcyodtCVgTAEaM2lGwxzypAb/OEX86scjVsDWH",
		"Qwhgl+PDxjDM+LW6bnhCzpL2ZkuliP+xf0DjhADnAyRnR0CDwJO5iUb7OS/RsGId",
		"3p+NmTysyRxWwqE7cFKQdBvgztIvqViwc9a5gPi81zTGXhkuSt3I9a2l+GJtxBKZ",
		"oe9DEFdcjGw7G6+PAfqYAlArranek5ID6VjsDFTTw0LfLHRdn3zdFAlVLSso8DTl",
		"+7hADyo6labKQkWhVcZjMI3I00n5L4/b9kLs34QZCb5qLm7S420/3o9mQemJ3s70",
		"rlqV0qFzAb1TU7d5+RRjcjNoJVplRsemd5278CPggMB8kAZNbYKvdILHsGPI/6Gp",
		"VdkJxpch1U1CSD+LbliqGMvetDak5X2bjJJuYgCZO7FQJIZV6gtvOUREbKtcOM88",
		"sFL7p4bMCtrRxtDMbv7IFCZTcLin8zSgbfZ7fX2RT4sEiPqoSdVyrUw1mW+7duKk",
		"Yw4+ot2O2nGrXr87ECICAR9G2W/7FJR1NGLzHLg=",
		"-----END CERTIFICATE-----",
	},
	JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"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",
		"Y8gMhSbb7NTB",
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
		p.second.reset();
	}
}
