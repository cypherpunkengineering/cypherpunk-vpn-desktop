#include "config.h"
#include "logger.h"
#include "path.h"
#include "win.h"

#include <fwpvi.h>
#include <fwpmu.h>
#include <accctrl.h>
#include <aclapi.h>

#pragma comment (lib, "fwpuclnt.lib")
#pragma comment (lib, "advapi32.lib")



struct WinFirewallFilter : FWPM_FILTER
{
	WinFirewallFilter();
};

typedef UINT64 WinFirewallFilterId;

class WinFirewallEngine
{
	HANDLE _engine;
public:
	WinFirewallEngine();
	~WinFirewallEngine();

	WinFirewallFilterId AddFilter(const WinFirewallFilter& filter);
	void DeleteFilter(WinFirewallFilterId id);
};


#define WFP_TRY(api, ...) \


WinFirewallFilter::WinFirewallFilter()
{
	memset(&filterKey, 0, sizeof(filterKey));
	displayData.name = L"Cypherpunk VPN Firewall";
	displayData.description = L"Implements the various firewall protection features of Cypherpunk VPN.";

}

WinFirewallEngine::WinFirewallEngine()
{
	if (DWORD error = FwpmEngineOpen(NULL, RPC_C_AUTHN_DEFAULT, NULL, NULL, &_engine))
	{
		PrintLastError(FwpmEngineOpen);
		throw std::exception("Unable to open filter engine");
	}
}

WinFirewallEngine::~WinFirewallEngine()
{
	FwpmEngineClose(_engine);
}

WinFirewallFilterId WinFirewallEngine::AddFilter(const WinFirewallFilter& filter)
{
	return 0;
}

void WinFirewallEngine::DeleteFilter(WinFirewallFilterId id)
{
	FwpmFilterDeleteById(_engine, id);
}

void win_firewall_configure(bool blockOtherApps, bool allowLAN)
{

}
