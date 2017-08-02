#include "config.h"
#include "win.h"
#include "path.h"
#include "serialization.h"

#include <vector>
#include <memory>
#include <cstdio>
#include <string>
#include <tchar.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <rpc.h>

#define _PRSHT_H_
#include <netcon.h>

#pragma comment (lib, "rpcrt4.lib")


void serialization::json_converter<GUID, JsonValue>::convert(GUID& dst, const JsonValue& src)
{
	WIN_CHECK_RESULT(UuidFromStringA, (reinterpret_cast<RPC_CSTR>(const_cast<char*>(src.AsString().c_str())), &dst));
}
void serialization::json_converter<JsonValue, GUID>::convert(JsonValue& dst, const GUID& src)
{
	RPC_CSTR str;
	WIN_CHECK_RESULT(UuidToStringA, (&src, &str));
	dst = std::string(reinterpret_cast<const char*>(str));
	WIN_CHECK_RESULT(RpcStringFreeA, (&str));
}


template<class CLASS, class INTERFACE> HRESULT CoCreateInstance(LPUNKNOWN pUnkOuter, DWORD dwClsContext, INTERFACE** ppv)
{ return ::CoCreateInstance(__uuidof(CLASS), pUnkOuter, dwClsContext, __uuidof(INTERFACE), (void**)ppv); }
template<class CLASS, class INTERFACE> HRESULT CoCreateInstance(LPUNKNOWN pUnkOuter, INTERFACE** ppv)
{ return ::CoCreateInstance(__uuidof(CLASS), pUnkOuter, CLSCTX_ALL, __uuidof(INTERFACE), (void**)ppv); }
template<class CLASS, class INTERFACE> HRESULT CoCreateInstance(DWORD dwClsContext, INTERFACE** ppv)
{ return ::CoCreateInstance(__uuidof(CLASS), NULL, dwClsContext, __uuidof(INTERFACE), (void**)ppv); }
template<class CLASS, class INTERFACE> HRESULT CoCreateInstance(INTERFACE** ppv)
{ return ::CoCreateInstance(__uuidof(CLASS), NULL, CLSCTX_ALL, __uuidof(INTERFACE), (void**)ppv); }
template<class INTERFACE> HRESULT CoCreateInstance(const IID& rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, INTERFACE** ppv)
{ return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(INTERFACE), (void**)ppv); }
template<class INTERFACE> HRESULT CoCreateInstance(const IID& rclsid, LPUNKNOWN pUnkOuter, INTERFACE** ppv)
{ return ::CoCreateInstance(rclsid, pUnkOuter, CLSCTX_ALL, __uuidof(INTERFACE), (void**)ppv); }
template<class INTERFACE> HRESULT CoCreateInstance(const IID& rclsid, DWORD dwClsContext, INTERFACE** ppv)
{ return ::CoCreateInstance(rclsid, NULL, dwClsContext, __uuidof(INTERFACE), (void**)ppv); }
template<class INTERFACE> HRESULT CoCreateInstance(const IID& rclsid, INTERFACE** ppv)
{ return ::CoCreateInstance(rclsid, NULL, CLSCTX_ALL, __uuidof(INTERFACE), (void**)ppv); }

template<class CLASS, class INTERFACE> INTERFACE* CoCreateInstance(LPUNKNOWN pUnkOuter, DWORD dwClsContext)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (__uuidof(CLASS), pUnkOuter, dwClsContext, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class CLASS, class INTERFACE> INTERFACE* CoCreateInstance(LPUNKNOWN pUnkOuter)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (__uuidof(CLASS), pUnkOuter, CLSCTX_ALL, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class CLASS, class INTERFACE> INTERFACE* CoCreateInstance(DWORD dwClsContext)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (__uuidof(CLASS), NULL, dwClsContext, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class CLASS, class INTERFACE> INTERFACE* CoCreateInstance()
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (__uuidof(CLASS), NULL, CLSCTX_ALL, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class INTERFACE> INTERFACE* CoCreateInstance(const IID& rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (rclsid, pUnkOuter, dwClsContext, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class INTERFACE> INTERFACE* CoCreateInstance(const IID& rclsid, LPUNKNOWN pUnkOuter)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (rclsid, pUnkOuter, CLSCTX_ALL, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class INTERFACE> INTERFACE* CoCreateInstance(const IID& rclsid, DWORD dwClsContext)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (rclsid, NULL, dwClsContext, __uuidof(INTERFACE), (void**)&pv)); return pv; }
template<class INTERFACE> INTERFACE* CoCreateInstance(const IID& rclsid)
{ INTERFACE* pv; WIN_CHECK_RESULT(CoCreateInstance, (rclsid, NULL, CLSCTX_ALL, __uuidof(INTERFACE), (void**)&pv)); return pv; }

template<class INTERFACE, class ORIGINAL = IUnknown> INTERFACE* com_cast(ORIGINAL* pUnknown)
{ INTERFACE* pv; CheckError(pUnknown->QueryInterface(__uuidof(INTERFACE), (void**)&pv), "QueryInterface" _D(, CURRENT_LOCATION)); return pv; }


template<typename First, typename... Args> struct GetLastType { typedef typename GetLastType<Args...>::TYPE TYPE; };
template<typename Last> struct GetLastType<Last> { typedef Last TYPE; };

template<class Interface, typename... FnArgs>
struct ComReturnHelper
{
	Interface* p;
	typedef decltype(*std::declval<typename GetLastType<FnArgs...>::TYPE>()) LastArg;
	HRESULT(__stdcall Interface::*fn)(FnArgs...);
	const char* name;
#ifdef _DEBUG
	const Location& loc;
#endif
	ComReturnHelper(Interface* p, HRESULT(__stdcall Interface::*fn)(FnArgs..., LastArg*) _D(, LOCATION_DECL)) : p(p), fn(fn) _D(loc(LOCATION_VAR)) {}
	template<typename... Args> LastArg operator()(Args&&... args)
	{
		LastArg arg;
		CheckError(p->fn(std::forward<Args>(args)..., &arg), name _D(, loc));
		return arg;
	}
};

template<class Interface, typename... Args>
static inline ComReturnHelper<Interface, Args...> ComWrapCheckedReturnArg(Interface* p, HRESULT(__stdcall Interface::*fn)(Args...), const char* name _D(, LOCATION_DECL))
{
	return ComReturnHelper<Interface, Args...>(p, fn, name _D(, LOCATION_VAR));
}



template<class LastArg, class INTERFACE, typename... Args>
static inline LastArg CheckCOMReturnLastArgument(INTERFACE* p, HRESULT (__stdcall INTERFACE::*fn)(Args..., LastArg*), const char* name _D(, LOCATION_DECL), Args&&... args)
{
	LastArg arg;
	CheckError(p->*fn(std::forward<Args>(args)..., &arg), name _D(, LOCATION_VAR));
	return arg;
}


template<class T> void SafeRelease(T*& p)
{
	IUnknown* u = p;
	p->Release();
	p = NULL;
}


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



std::vector<win_tap_adapter> win_get_tap_adapters(bool include_plain_tap)
{
	std::vector<win_tap_adapter> result;

	int iterations = 0;

	ULONG error;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_WINS_INFO | GAA_FLAG_INCLUDE_GATEWAYS;
	ULONG size = 15000;

	IP_ADAPTER_ADDRESSES* addresses = NULL;

	do
	{
		if (NULL == (addresses = (IP_ADAPTER_ADDRESSES*)malloc(size)))
			THROW_WIN32EXCEPTION(ERROR_NOT_ENOUGH_MEMORY, malloc);
		
		if ((error = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addresses, &size)) != ERROR_BUFFER_OVERFLOW)
			break;

		free(addresses);
		addresses = NULL;

	} while (++iterations < 3);

	if (error != ERROR_SUCCESS)
		THROW_WIN32EXCEPTION(error, GetAdaptersAddresses);
	
	for (auto address = addresses; address; address = address->Next)
	{
		if (address->Description && wcsstr(address->Description, L"Cypherpunk Privacy Virtual Network Adapter") != NULL)
		{
			win_tap_adapter adapter;
			adapter.guid = address->AdapterName;
			adapter.luid = address->Luid.Value;
			adapter.adapter_name = address->Description;
			adapter.connection_name = address->FriendlyName;
			adapter.is_custom_tap = true;
			result.push_back(std::move(adapter));
		}
	}
	if (include_plain_tap)
	{
		for (auto address = addresses; address; address = address->Next)
		{
			if (address->Description && wcsstr(address->Description, L"TAP-Windows Adapter") != NULL)
			{
				win_tap_adapter adapter;
				adapter.guid = address->AdapterName;
				adapter.luid = address->Luid.Value;
				adapter.adapter_name = address->Description;
				adapter.connection_name = address->FriendlyName;
				adapter.is_custom_tap = false;
				result.push_back(std::move(adapter));
			}
		}
	}

	free(addresses);



	try
	{
		auto nsm = CoCreateInstance<NetSharingManager, INetSharingManager>();
		auto coll = ComWrapCheckedReturnArg(nsm, &INetSharingManager::get_EnumEveryConnection, "get_EnumEveryCollection", CURRENT_LOCATION)();

		/*
		INetSharingEveryConnectionCollection* coll = NULL;
		WIN_CHECK_COM(nsm, get_EnumEveryConnection, (&coll));
		IUnknown* u = NULL;
		WIN_CHECK_COM(coll, get__NewEnum, (&u));
		*/
	}
	catch (const Win32Exception& e)
	{
	}

	INetSharingManager* nsm = NULL;
	if (!(error = CoCreateInstance(__uuidof(NetSharingManager), NULL, CLSCTX_ALL, __uuidof(INetSharingManager), (void**)&nsm)))
	{
		INetSharingEveryConnectionCollection* coll = NULL;
		if (!(error = nsm->get_EnumEveryConnection(&coll)))
		{
			IUnknown* u = NULL;
			if (!(error = coll->get__NewEnum(&u)))
			{
				IEnumVARIANT* ev = NULL;
				if (!(error = u->QueryInterface(&ev)))
				{
					VARIANT v;
					VariantInit(&v);
					while (!(error = ev->Next(1, &v, NULL)) && !found)
					{
						if (V_VT(&v) == VT_UNKNOWN)
						{
							INetConnection* nc = NULL;
							if (!(error = V_UNKNOWN(&v)->QueryInterface(&nc)))
							{
								
							}
						}
					}

					SafeRelease(ev);
				}
				SafeRelease(u);
			}
			SafeRelease(coll);
		}
		SafeRelease(nsm);
	}


	










	return std::move(result);
}

static BOOL win_tap_install(LPTSTR cmdline)
{
	std::tstring tap_path = convert<TCHAR>(GetPath(TapDriverDir));
	std::tstring tap_install = convert<TCHAR>(GetFile(TapInstallExecutable));

	STARTUPINFO si = { sizeof(si), 0 };
	PROCESS_INFORMATION pi;
	if (CreateProcess(tap_install.c_str(), cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, tap_path.c_str(), &si, &pi))
	{
		DWORD exit_code;
		return (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0 && GetExitCodeProcess(pi.hProcess, &exit_code) && exit_code == 0);
	}
	else
		PrintLastError(CreateProcess);
	return FALSE;
}

static win_tap_adapter win_tap_add(std::vector<win_tap_adapter>* existing = nullptr)
{
	std::vector<win_tap_adapter> local_existing;
	if (!existing)
	{
		local_existing = win_get_tap_adapters(false);
		existing = &local_existing;
	}

	if (win_tap_install(_T("tapinstall.exe install OemVista.inf tap91337")))
	{
		std::vector<win_tap_adapter> current = win_get_tap_adapters(false);
		for (auto& adapter : current)
		{
			if (std::find_if(existing->begin(), existing->end(), [&](const win_tap_adapter& a) { return a.guid == adapter.guid; }) == existing->end())
			{
				// Found first adapter which didn't previously exist; assume there are no others.
				if (existing != &local_existing)
				{
					std::swap(current, *existing);
				}
				return std::move(adapter);
			}
		}
	}
	THROW_WIN32EXCEPTION(ERROR_DEV_NOT_EXIST, tapinstall.exe);
}

static BOOL win_tap_add2(std::vector<win_tap_adapter>* existing = nullptr)
{
	if (win_tap_install(_T("tapinstall.exe install OemVista.inf tap91337")))
	{
		if (existing)
		{
			auto adapters = win_get_tap_adapters(false);
			std::vector<win_tap_adapter> added;
			for (auto& adapter : adapters)
			{
				if (std::find_if(existing->begin(), existing->end(), [&](const win_tap_adapter& a) { return a.guid == adapter.guid; }) == existing->end())
				{
					// Adapter didn't already exist
					LOG(INFO) << "Renaming \"" << adapter.connection_name << "\" to \"Cypherpunk Privacy\"";
					added.push_back(std::move(adapter));
				}
			}
			for (auto& adapter : added)
			{
				existing->push_back(std::move(adapter));
			}
		}
		return TRUE;
	}
	return FALSE;
}

static BOOL win_tap_rename(const win_tap_adapter& adapter, PCWSTR name)
{
	// TODO: Either tweak registry or call netsh interface set interface name="{adapter.connection_name}" newname="Cypherpunk Privacy"



	return TRUE;
}

BOOL win_install_tap_adapter(int argc, TCHAR **argv)
{
	auto existing = win_get_tap_adapters(false);
	try
	{
		if (argc >= 1) {
			int count = 1;
			try { count = std::stoi(argv[0]); }
			catch (...) { return FALSE; }
			if ((int)existing.size() >= count)
				return TRUE;
			count -= (int)existing.size();
			while (count--)
			{
				win_tap_rename(win_tap_add(&existing), L"Cypherpunk Privacy"); 
			}
			return TRUE;
		}
		win_tap_rename(win_tap_add(&existing), L"Cypherpunk Privacy");
		return TRUE;
	}
	catch (const Win32Exception& e)
	{
		LOG(ERROR) << e;
		return FALSE;
	}
}

BOOL win_uninstall_tap_adapters()
{
	return win_tap_install(_T("tapinstall.exe remove tap91337"));
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
			PrintError(malloc, 0);
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
		while (len > 0 && isspace(errorText[len - 1]) || errorText[len - 1] == '.') len--;
		std::string result = convert<char>(errorText, errorText + len);
		if (errorText)
			LocalFree(errorText);
		return std::move(result);
	}
	return "Unknown error";
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
