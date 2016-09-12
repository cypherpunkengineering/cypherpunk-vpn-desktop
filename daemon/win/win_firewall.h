#pragma once

#include "win.h"
#include "util.h"

#include <fwpvi.h>
#include <fwpmu.h>

#include <algorithm>

typedef UINT64 FWFilterId;

struct FWFilter : public FWPM_FILTER, private noncopyable
{
	FWFilter();
	operator const GUID&() const { return filterKey; }
	operator const FWFilterId&() const { return filterId; }
};

class FWEngine : public Win32Handle<FWEngine>
{
public:
	FWEngine();

	static void Close(HANDLE handle) { FwpmEngineClose(handle); }

	void InstallProvider();
	void UninstallProvider();

	FWFilter& AddFilter(FWFilter& filter);
	FWFilter&& AddFilter(FWFilter&& filter);
	void RemoveFilter(FWFilterId filterId);
	void RemoveFilter(const GUID& filterKey);

	template<typename ITERATOR>
	inline size_t RemoveFilters(ITERATOR&& begin, ITERATOR&& end);

	size_t RemoveAllFilters();
	size_t RemoveAllFilters(const GUID& layerKey);
};

class FWTransaction
{
	FWEngine& _engine;
	bool _closed;
public:
	FWTransaction(FWEngine& engine);
	~FWTransaction();

	void Commit();
	void Abort();
};


template<typename ITERATOR>
inline size_t FWEngine::RemoveFilters(ITERATOR&& begin, ITERATOR&& end)
{
	size_t result = 0;
	std::for_each(std::forward<ITERATOR>(begin), std::forward<ITERATOR>(end), [&](const auto& f) {
		RemoveFilter(f);
		result++;
	});
	return result;
}




template<FWP_DATA_TYPE TYPE> struct FWValueAccessor { };
#define DECLARE_VALUE_OVERLOAD(i, t, m) template<> struct FWValueAccessor<i> { typedef t type; template<typename B> static inline type& get(B& b) { return b.m; } };
DECLARE_VALUE_OVERLOAD(FWP_UINT8, UINT8, uint8)
DECLARE_VALUE_OVERLOAD(FWP_UINT16, UINT16, uint16)
DECLARE_VALUE_OVERLOAD(FWP_UINT32, UINT32, uint32)
DECLARE_VALUE_OVERLOAD(FWP_UINT64, UINT64*, uint64)
DECLARE_VALUE_OVERLOAD(FWP_INT8, INT8, int8)
DECLARE_VALUE_OVERLOAD(FWP_INT16, INT16, int16)
DECLARE_VALUE_OVERLOAD(FWP_INT32, INT32, int32)
DECLARE_VALUE_OVERLOAD(FWP_INT64, INT64*, int64)
DECLARE_VALUE_OVERLOAD(FWP_FLOAT, float, float32)
DECLARE_VALUE_OVERLOAD(FWP_DOUBLE, double*, double64)
DECLARE_VALUE_OVERLOAD(FWP_BYTE_ARRAY16_TYPE, FWP_BYTE_ARRAY16*, byteArray16)
DECLARE_VALUE_OVERLOAD(FWP_BYTE_BLOB_TYPE, FWP_BYTE_BLOB*, byteBlob)
DECLARE_VALUE_OVERLOAD(FWP_SID, SID*, sid)
DECLARE_VALUE_OVERLOAD(FWP_SECURITY_DESCRIPTOR_TYPE, FWP_BYTE_BLOB*, sd)
DECLARE_VALUE_OVERLOAD(FWP_TOKEN_INFORMATION_TYPE, FWP_TOKEN_INFORMATION*, tokenInformation)
DECLARE_VALUE_OVERLOAD(FWP_TOKEN_ACCESS_INFORMATION_TYPE, FWP_BYTE_BLOB*, tokenAccessInformation)
DECLARE_VALUE_OVERLOAD(FWP_UNICODE_STRING_TYPE, LPWSTR, unicodeString)
DECLARE_VALUE_OVERLOAD(FWP_BYTE_ARRAY6_TYPE, FWP_BYTE_ARRAY6*, byteArray6)
DECLARE_VALUE_OVERLOAD(FWP_V4_ADDR_MASK, FWP_V4_ADDR_AND_MASK*, v4AddrMask)
DECLARE_VALUE_OVERLOAD(FWP_V6_ADDR_MASK, FWP_V6_ADDR_AND_MASK*, v6AddrMask)
DECLARE_VALUE_OVERLOAD(FWP_RANGE_TYPE, FWP_RANGE*, rangeValue)
#undef DECLARE_VALUE_OVERLOAD


enum FWDirection
{
	Incoming,
	Outgoing
};

template<FWP_ACTION_TYPE ACTION, FWDirection DIRECTION, FWP_IP_VERSION VERSION>
struct FWBasicFilter : public FWFilter
{
	FWBasicFilter()
	{
		if (DIRECTION == Incoming)
			layerKey = (VERSION == FWP_IP_VERSION_V6) ? FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 : FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
		else if (DIRECTION == Outgoing)
			layerKey = (VERSION == FWP_IP_VERSION_V6) ? FWPM_LAYER_ALE_AUTH_CONNECT_V6 : FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		action.type = ACTION;
	}
	FWBasicFilter& SetWeight(UINT8 weight) { this->weight.uint8 = weight; return *this; }
};

template<size_t NUM_CONDITIONS, FWP_ACTION_TYPE ACTION, FWDirection DIRECTION, FWP_IP_VERSION VERSION>
struct FWConditionFilter : public FWBasicFilter<ACTION, DIRECTION, VERSION>
{
	FWPM_FILTER_CONDITION _conds[NUM_CONDITIONS];
	FWConditionFilter()
	{
		memset(_conds, 0, sizeof(_conds));
		numFilterConditions = NUM_CONDITIONS;
		filterCondition = _conds;
	}
	template<FWP_DATA_TYPE TYPE> void SetCondition(size_t index, const GUID fieldKey, FWP_MATCH_TYPE matchType, const typename FWValueAccessor<TYPE>::type& value)
	{
		_conds[index].fieldKey = fieldKey;
		_conds[index].matchType = matchType;
		_conds[index].conditionValue.type = TYPE;
		FWValueAccessor<TYPE>::get(_conds[index].conditionValue) = value;
	}
};

template<FWDirection DIRECTION, FWP_IP_VERSION VERSION> struct AllowLocalHostFilter;

template<FWDirection DIRECTION>
struct AllowLocalHostFilter<DIRECTION, FWP_IP_VERSION_V4> : public FWConditionFilter<1, FWP_ACTION_PERMIT, DIRECTION, FWP_IP_VERSION_V4>
{
	FWP_V4_ADDR_AND_MASK _addr;
	AllowLocalHostFilter()
	{
		_addr.addr = 127 << 24 | 1;
		_addr.mask = 0xFFFFFFFF;
		SetCondition<FWP_V4_ADDR_MASK>(0, FWPM_CONDITION_IP_REMOTE_ADDRESS, FWP_MATCH_EQUAL, &_addr);
		weight.uint8 = 10;
	}
};

template<FWDirection DIRECTION>
struct AllowLocalHostFilter<DIRECTION, FWP_IP_VERSION_V6> : public FWConditionFilter<1, FWP_ACTION_PERMIT, DIRECTION, FWP_IP_VERSION_V6>
{
	FWP_V6_ADDR_AND_MASK _addr;
	AllowLocalHostFilter()
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.addr[15] = 1;
		_addr.prefixLength = 128;
		SetCondition<FWP_V6_ADDR_MASK>(0, FWPM_CONDITION_IP_REMOTE_ADDRESS, FWP_MATCH_EQUAL, &_addr);
		weight.uint8 = 10;
	}
};

template<FWP_IP_VERSION VERSION> struct AllowDHCPFilter;

template<>
struct AllowDHCPFilter<FWP_IP_VERSION_V4> : public FWConditionFilter<2, FWP_ACTION_PERMIT, Outgoing, FWP_IP_VERSION_V4>
{
	AllowDHCPFilter()
	{
		SetCondition<FWP_UINT16>(0, FWPM_CONDITION_IP_LOCAL_PORT, FWP_MATCH_EQUAL, 68);
		SetCondition<FWP_UINT16>(1, FWPM_CONDITION_IP_REMOTE_PORT, FWP_MATCH_EQUAL, 67);
		weight.uint8 = 10;
	}
};

template<>
struct AllowDHCPFilter<FWP_IP_VERSION_V6> : public FWConditionFilter<2, FWP_ACTION_PERMIT, Outgoing, FWP_IP_VERSION_V6>
{
	AllowDHCPFilter()
	{
		SetCondition<FWP_UINT16>(0, FWPM_CONDITION_IP_LOCAL_PORT, FWP_MATCH_EQUAL, 546);
		SetCondition<FWP_UINT16>(1, FWPM_CONDITION_IP_REMOTE_PORT, FWP_MATCH_EQUAL, 547);
		weight.uint8 = 10;
	}
};

template<FWP_IP_VERSION VERSION>
struct AllowDNSFilter : public FWConditionFilter<1, FWP_ACTION_PERMIT, Outgoing, VERSION>
{
	AllowDNSFilter()
	{
		SetCondition<FWP_UINT16>(0, FWPM_CONDITION_IP_REMOTE_PORT, FWP_MATCH_EQUAL, 53);
		weight.uint8 = 10;
	}
};

template<FWDirection DIRECTION, FWP_IP_VERSION VERSION>
struct AllowAppFilter : public FWConditionFilter<1, FWP_ACTION_PERMIT, DIRECTION, VERSION>
{
	FWP_BYTE_BLOB* _appBlob = NULL;
	AllowAppFilter(const std::string& app_path)
	{
		WIN_CHECK_RESULT(FwpmGetAppIdFromFileName, (convert<wchar_t>(app_path).c_str(), &_appBlob));
		SetCondition<FWP_BYTE_BLOB_TYPE>(0, FWPM_CONDITION_ALE_APP_ID, FWP_MATCH_EQUAL, _appBlob);
		weight.uint8 = 11;
	}
	~AllowAppFilter()
	{
		FwpmFreeMemory((void**)&_appBlob);
	}
};

template<FWDirection DIRECTION, FWP_IP_VERSION VERSION>
struct BlockAllFilter : public FWBasicFilter<FWP_ACTION_BLOCK, DIRECTION, VERSION>
{
	BlockAllFilter()
	{
		weight.uint8 = 0;
	}
};

