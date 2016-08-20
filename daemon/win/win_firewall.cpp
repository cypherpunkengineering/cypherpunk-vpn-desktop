#include "config.h"
#include "logger.h"
#include "path.h"
#include "win.h"
#include "util.h"

#include <fwpvi.h>
#include <fwpmu.h>
#include <accctrl.h>
#include <aclapi.h>

#pragma comment (lib, "fwpuclnt.lib")
#pragma comment (lib, "advapi32.lib")


static wchar_t DEFAULT_FIREWALL_NAME[] = L"Cypherpunk VPN Firewall";
static wchar_t DEFAULT_FIREWALL_DESCRIPTION[] = L"Implements the various firewall protection features of Cypherpunk VPN.";

// {4C60C564-E8D2-4D21-B717-C343999EFC59}
static GUID g_wfp_provider_key =
{ 0x4c60c564, 0xe8d2, 0x4d21,{ 0xb7, 0x17, 0xc3, 0x43, 0x99, 0x9e, 0xfc, 0x59 } };

// {7BD5536B-B532-4C0B-BF38-DBB9DAE739E1}
static const GUID g_wfp_sublayer_key =
{ 0x7bd5536b, 0xb532, 0x4c0b,{ 0xbf, 0x38, 0xdb, 0xb9, 0xda, 0xe7, 0x39, 0xe1 } };



struct WinFirewallFilter : public FWPM_FILTER { WinFirewallFilter(); };


typedef UINT64 WinFirewallFilterId;

DECLARE_HANDLE_CLOSER(WinFirewallEngineCloser, FwpmEngineClose)
class WinFirewallEngine : public Win32Handle<WinFirewallEngineCloser>
{
public:
	WinFirewallEngine();

	void Install();
	void Uninstall();

	WinFirewallFilterId AddFilter(const WinFirewallFilter& filter);
	void DeleteFilter(WinFirewallFilterId id);
	void DeleteFilter(const GUID& guid);

	void DeleteAllFilters();
};



WinFirewallFilter::WinFirewallFilter()
{
	memset(&filterKey, 0, sizeof(filterKey));
	displayData.name = DEFAULT_FIREWALL_NAME;
	displayData.description = DEFAULT_FIREWALL_DESCRIPTION;
	providerKey = &g_wfp_provider_key;
	subLayerKey = g_wfp_sublayer_key;
}


struct AllowDHCPFilter : public WinFirewallFilter
{
	FWPM_FILTER_CONDITION _conds[2];
	AllowDHCPFilter()
	{
		memset(_conds, 0, sizeof(_conds));
		flags |= FWPM_FILTER_FLAG_INDEXED;
	}
};


WinFirewallEngine::WinFirewallEngine()
{
	WIN_CHECK_RESULT(FwpmEngineOpen, (NULL, RPC_C_AUTHN_DEFAULT, NULL, NULL, &_handle));
}

void WinFirewallEngine::Install()
{
	// Ensure the WFP provider exists
	try
	{
		FWPM_PROVIDER* existing_provider = NULL;
		WIN_CHECK_RESULT(FwpmProviderGetByKey, (_handle, &g_wfp_provider_key, &existing_provider));
		FwpmFreeMemory((void**)&existing_provider);
	}
	catch (const Win32Exception& e)
	{
		if (e.code() != FWP_E_PROVIDER_NOT_FOUND)
			throw;
		FWPM_PROVIDER provider = {
			g_wfp_provider_key,
			{ DEFAULT_FIREWALL_NAME, DEFAULT_FIREWALL_DESCRIPTION },
			FWPM_PROVIDER_FLAG_PERSISTENT,
			{ 0, NULL },
			NULL
		};
		WIN_CHECK_RESULT(FwpmProviderAdd, (_handle, &provider, NULL));
	}
	// Ensure the WFP sublayer exists
	try
	{
		FWPM_SUBLAYER* existing_sublayer = NULL;
		WIN_CHECK_RESULT(FwpmSubLayerGetByKey, (_handle, &g_wfp_sublayer_key, &existing_sublayer));
		FwpmFreeMemory((void**)&existing_sublayer);
	}
	catch (const Win32Exception& e)
	{
		if (e.code() != FWP_E_SUBLAYER_NOT_FOUND)
			throw;
		FWPM_SUBLAYER sublayer = {
			g_wfp_sublayer_key,
			{ DEFAULT_FIREWALL_NAME, DEFAULT_FIREWALL_DESCRIPTION },
			FWPM_SUBLAYER_FLAG_PERSISTENT,
			&g_wfp_provider_key,
			{ 0, NULL },
			9000
		};
		WIN_CHECK_RESULT(FwpmSubLayerAdd, (_handle, &sublayer, NULL));
	}
}

void WinFirewallEngine::Uninstall()
{
	FwpmSubLayerDeleteByKey(_handle, &g_wfp_sublayer_key);
	FwpmProviderDeleteByKey(_handle, &g_wfp_provider_key);
}

WinFirewallFilterId WinFirewallEngine::AddFilter(const WinFirewallFilter& filter)
{
	return 0;
}

void WinFirewallEngine::DeleteFilter(WinFirewallFilterId id)
{
	FwpmFilterDeleteById(_handle, id);
}

void WinFirewallEngine::DeleteAllFilters()
{

}

enum WinFirewallFlags
{
	FW_BlockIncomingConnections = 0x01,
	FW_BlockOutgoingConnections = 0x02,
	FW_AllowLANConnection = 0x10,
	FW_AllowIPv6 = 0x20,
};

void win_firewall_configure(unsigned long flags, const GUID& vpn_interface)
{

}

void win_firewall_test()
{
	WinFirewallEngine fw;
	fw.Install();
	fw.Uninstall();
	LOG(INFO) << "Yay";
}
