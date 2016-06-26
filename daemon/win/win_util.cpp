#include "win.h"

#include <vector>
#include <memory>

#include <stdio.h>

#include <winsock2.h>
#include <windows.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

template<typename T> struct IP_LIST : public T
{
	typedef T NODE;
};

#define DEFINE_LIST(TYPE) struct TYPE##_HEAD : public IP_LIST<TYPE> {};
DEFINE_LIST(IP_ADAPTER_ADDRESSES);
DEFINE_LIST(IP_ADAPTER_UNICAST_ADDRESS);
DEFINE_LIST(IP_ADAPTER_ANYCAST_ADDRESS);
DEFINE_LIST(IP_ADAPTER_MULTICAST_ADDRESS);
DEFINE_LIST(IP_ADAPTER_DNS_SERVER_ADDRESS);
DEFINE_LIST(IP_ADAPTER_PREFIX);
DEFINE_LIST(IP_ADAPTER_WINS_SERVER_ADDRESS);
DEFINE_LIST(IP_ADAPTER_GATEWAY_ADDRESS);
DEFINE_LIST(IP_ADAPTER_DNS_SUFFIX);
#undef DEFINE_LIST

struct IPv4 { unsigned char b[4]; } _ipv4_dummy;
union IPv6 { unsigned char b[16]; unsigned short w[8]; } _ipv6_dummy;
union IPv4M2P { unsigned char b[4]; unsigned int i; } _ipv4m2p_dummy;
struct HEX16 { unsigned char b[2]; } _hex_dummy;
char HEX[256][4] = { 0 };



std::vector<win_tap_adapter> win_get_tap_adapters()
{
	std::vector<win_tap_adapter> result;

	ULONG error;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_WINS_INFO | GAA_FLAG_INCLUDE_GATEWAYS;
	ULONG size = 0;
	if ((error = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &size)) == ERROR_BUFFER_OVERFLOW)
	{
		if (auto addresses = (IP_ADAPTER_ADDRESSES*)malloc(size))
		{
			if ((error = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addresses, &size)) == ERROR_SUCCESS)
			{
				for (auto address = addresses; address; address = address->Next)
				{
					if (address->Description && wcsstr(address->Description, L"TAP-Windows Adapter") != NULL)
					{
						win_tap_adapter adapter;
						adapter.guid = address->AdapterName;
						result.push_back(std::move(adapter));
					}
				}
			}
			else
				PrintError(GetAdaptersAddresses, error);
			free(addresses);
		}
		else
			PrintError("malloc", 0);
	}
	else
		PrintError(GetAdaptersAddresses, error);

	return std::move(result);
}

void win_get_ipv4_routing_table()
{
	DWORD error;
	ULONG size = 0;
	if ((error = GetIpForwardTable(NULL, &size, TRUE)) == ERROR_INSUFFICIENT_BUFFER)
	{
		if (auto table = (MIB_IPFORWARDTABLE*)malloc(size))
		{
			if ((error = GetIpForwardTable(table, &size, TRUE)) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; i++)
				{
					table->table[i];
				}
			}
			else
				PrintError(GetIpForwardTable, error);
			free(table);
		}
		else
			PrintError("malloc", 0);
	}
	else
		PrintError(GetIpForwardTable, error);
}
