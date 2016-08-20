#include "config.h"
#include "win_firewall.h"

#include "logger.h"
#include "path.h"
#include "util.h"

#include <accctrl.h>
#include <aclapi.h>
#include <rpc.h>

#pragma comment (lib, "fwpuclnt.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "rpcrt4.lib")


static wchar_t DEFAULT_FIREWALL_NAME[] = L"Cypherpunk VPN Firewall";
static wchar_t DEFAULT_FIREWALL_DESCRIPTION[] = L"Implements the various firewall protection features of Cypherpunk VPN.";

static FWPM_PROVIDER g_wfp_provider = {
	// {4C60C564-E8D2-4D21-B717-C343999EFC59}
	{ 0x4c60c564, 0xe8d2, 0x4d21,{ 0xb7, 0x17, 0xc3, 0x43, 0x99, 0x9e, 0xfc, 0x59 } },
	{ DEFAULT_FIREWALL_NAME, DEFAULT_FIREWALL_DESCRIPTION },
	FWPM_PROVIDER_FLAG_PERSISTENT,
	{ 0, NULL },
	NULL
};

static FWPM_SUBLAYER g_wfp_sublayer = {
	// {7BD5536B-B532-4C0B-BF38-DBB9DAE739E1}
	{ 0x7bd5536b, 0xb532, 0x4c0b,{ 0xbf, 0x38, 0xdb, 0xb9, 0xda, 0xe7, 0x39, 0xe1 } },
	{ DEFAULT_FIREWALL_NAME, DEFAULT_FIREWALL_DESCRIPTION },
	FWPM_SUBLAYER_FLAG_PERSISTENT,
	&g_wfp_provider.providerKey,
	{ 0, NULL },
	9000
};


FWFilter::FWFilter()
{
	memset(this, 0, sizeof(FWPM_FILTER));
	UuidCreate(&filterKey);
	displayData.name = DEFAULT_FIREWALL_NAME;
	displayData.description = DEFAULT_FIREWALL_DESCRIPTION;
	flags |= FWPM_FILTER_FLAG_PERSISTENT | FWPM_FILTER_FLAG_INDEXED;
	providerKey = &g_wfp_provider.providerKey;
	subLayerKey = g_wfp_sublayer.subLayerKey;
	weight.type = FWP_UINT8;
}

FWEngine::FWEngine()
{
	WIN_CHECK_RESULT(FwpmEngineOpen, (NULL, RPC_C_AUTHN_DEFAULT, NULL, NULL, &_handle));
}

void FWEngine::InstallProvider()
{
	// Ensure our WFP provider exists
	try
	{
		FWPM_PROVIDER* existing_provider = NULL;
		WIN_CHECK_RESULT(FwpmProviderGetByKey, (_handle, &g_wfp_provider.providerKey, &existing_provider));
		FwpmFreeMemory((void**)&existing_provider);
	}
	catch (const Win32Exception& e)
	{
		if (e.code() != FWP_E_PROVIDER_NOT_FOUND)
			throw;
		WIN_CHECK_RESULT(FwpmProviderAdd, (_handle, &g_wfp_provider, NULL));
	}
	// Ensure our WFP sublayer exists
	try
	{
		FWPM_SUBLAYER* existing_sublayer = NULL;
		WIN_CHECK_RESULT(FwpmSubLayerGetByKey, (_handle, &g_wfp_sublayer.subLayerKey, &existing_sublayer));
		FwpmFreeMemory((void**)&existing_sublayer);
	}
	catch (const Win32Exception& e)
	{
		if (e.code() != FWP_E_SUBLAYER_NOT_FOUND)
			throw;
		WIN_CHECK_RESULT(FwpmSubLayerAdd, (_handle, &g_wfp_sublayer, NULL));
	}
}

void FWEngine::UninstallProvider()
{
	try { FwpmSubLayerDeleteByKey(_handle, &g_wfp_sublayer.subLayerKey); }
	catch (const Win32Exception& e) { if (e.code() != FWP_E_SUBLAYER_NOT_FOUND) throw; }
	try { FwpmProviderDeleteByKey(_handle, &g_wfp_provider.providerKey); }
	catch (const Win32Exception& e) { if (e.code() != FWP_E_PROVIDER_NOT_FOUND) throw; }
}

FWTransaction::FWTransaction(FWEngine& engine)
	: _engine(engine), _closed(false)
{
	WIN_CHECK_RESULT(FwpmTransactionBegin, (_engine, 0));
}

FWTransaction::~FWTransaction()
{
	if (!_closed) Abort();
}

void FWTransaction::Commit()
{
	WIN_CHECK_RESULT(FwpmTransactionCommit, (_engine));
	_closed = true;
}

void FWTransaction::Abort()
{
	WIN_CHECK_RESULT(FwpmTransactionAbort, (_engine));
	_closed = true;
}

FWFilter& FWEngine::AddFilter(FWFilter& filter)
{
	FWFilterId id = 0;
	WIN_CHECK_RESULT(FwpmFilterAdd, (_handle, &filter, NULL, &id));
	filter.filterId = id;
	return filter;
}

FWFilter&& FWEngine::AddFilter(FWFilter&& filter)
{
	FWFilterId id = 0;
	WIN_CHECK_RESULT(FwpmFilterAdd, (_handle, &filter, NULL, &id));
	filter.filterId = id;
	return std::move(filter);
}

void FWEngine::RemoveFilter(FWFilterId filterId)
{
	WIN_CHECK_RESULT(FwpmFilterDeleteById, (_handle, filterId));
}

void FWEngine::RemoveFilter(const GUID& filterKey)
{
	WIN_CHECK_RESULT(FwpmFilterDeleteByKey, (_handle, &filterKey));
}

size_t FWEngine::RemoveAllFilters()
{
	size_t result = 0;
	result += RemoveAllFilters(FWPM_LAYER_ALE_AUTH_CONNECT_V4);
	result += RemoveAllFilters(FWPM_LAYER_ALE_AUTH_CONNECT_V6);
	result += RemoveAllFilters(FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4);
	result += RemoveAllFilters(FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6);
	return result;
}

size_t FWEngine::RemoveAllFilters(const GUID& layerKey)
{
	size_t result = 0;
	FWPM_FILTER_ENUM_TEMPLATE search = { 0 };
	search.providerKey = &g_wfp_provider.providerKey;
	search.layerKey = layerKey;
	search.enumType = FWP_FILTER_ENUM_OVERLAPPING;
	search.actionMask = 0xFFFFFFFF;
	HANDLE enumHandle = NULL;
	WIN_CHECK_RESULT(FwpmFilterCreateEnumHandle, (_handle, &search, &enumHandle));
	FINALLY({ FwpmFilterDestroyEnumHandle(_handle, enumHandle); });

	FWPM_FILTER** entries;
	UINT32 count;
	do
	{
		WIN_CHECK_RESULT(FwpmFilterEnum, (_handle, enumHandle, 100, &entries, &count));
		for (UINT32 i = 0; i < count; i++)
		{
			RemoveFilter(entries[i]->filterId);
			result++;
		}
		FwpmFreeMemory((void**)&entries);
	} while (count == 100);

	return result;
}

