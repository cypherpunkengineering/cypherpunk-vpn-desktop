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


std::string GetLastErrorString(DWORD error)
{
	LPTSTR errorText = NULL;

	DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL, error, 0, (LPTSTR)&errorText, 0, NULL);

	if (len && errorText)
	{
		while (len > 0 && errorText[len - 1] == ' ') len--;
		std::string result = convert<char>(errorText, errorText + len);
		if (errorText)
			LocalFree(errorText);
		return std::move(result);
	}
	return "";
}

std::ostream& operator<<(std::ostream& os, const LastError& err)
{
	std::ios::fmtflags f(os.flags());
	os << err._op << " failed with error " << std::hex << std::showbase /*<< std::setfill('0') << std::internal << std::setw(10)*/ << err._error << " - " << GetLastErrorString(err._error);
	os.flags(f);
	return os;
}

template<> std::basic_string<char> convert<>(const wchar_t* wbegin, const wchar_t* const wend)
{
	if (wbegin == wend)
		return std::string();

	int wlen = wend - wbegin;

	int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wbegin, wlen, NULL, 0, NULL, NULL);

	if (!len || len == 0xFFFD)
		throw std::invalid_argument("Unable to convert wide string to UTF8");

	std::string result;
	result.resize(len);
	WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wbegin, wlen, &result[0], len, NULL, NULL);

	return std::move(result);
}

template<> std::basic_string<wchar_t> convert<>(const char* begin, const char* const end)
{
	if (begin == end)
		return std::wstring();

	int len = end - begin;

	int wlen = MultiByteToWideChar(CP_UTF8, 0, begin, len, NULL, 0);

	if (!wlen || wlen == 0xFFFD)
		throw std::invalid_argument("Unable to convert UTF8 to wide string");

	std::wstring result;
	result.resize(wlen);
	MultiByteToWideChar(CP_UTF8, 0, begin, len, &result[0], wlen);

	return std::move(result);
}

std::ostream& operator<<(std::ostream& os, const std::wstring& str)
{
	return os << convert<char>(str);
}
std::ostream& operator<<(std::ostream& os, const wchar_t* str)
{
	return os << convert<char>(str);
}
std::wostream& operator<<(std::wostream& os, const std::string& str)
{
	return os << convert<wchar_t>(str);
}
std::wostream& operator<<(std::wostream& os, const char* str)
{
	return os << convert<wchar_t>(str);
}

#ifndef _WIN64
bool IsWin64()
{
	static bool is_wow64 = []() {
		BOOL res = FALSE;
		return IsWow64Process(GetCurrentProcess(), &res) && res;
	}();
	return is_wow64;
}
#endif
