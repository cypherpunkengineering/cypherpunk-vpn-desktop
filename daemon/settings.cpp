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


void Config::ReadFromDisk()
{
	NativeJsonObject::ReadFromDisk(GetFile(SettingsFile), "config");
}
void Config::WriteToDisk() const
{
	NativeJsonObject::WriteToDisk(GetFile(SettingsFile, EnsureExists), "config");
}

void Account::ReadFromDisk()
{
	NativeJsonObject::ReadFromDisk(GetFile(SettingsFile), "account");
}
void Account::WriteToDisk() const
{
	NativeJsonObject::WriteToDisk(GetFile(SettingsFile, EnsureExists), "account");
}

void Settings::ReadFromDisk()
{
	NativeJsonObject::ReadFromDisk(GetFile(SettingsFile));
}
void Settings::WriteToDisk() const
{
	NativeJsonObject::WriteToDisk(GetFile(SettingsFile, EnsureExists));
}



static const JsonArray g_certificate_authorities {
	JsonArray {
		"-----BEGIN CERTIFICATE-----",
		"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",
		"dsj1LEFcf8DWaj19Dz099BxQgw==",
		"-----END CERTIFICATE-----",
	},
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

