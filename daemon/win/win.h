#pragma once

#include "config.h"
#include "daemon.h"
#include "logger.h"

#include <vector>
#include <string>


std::string GetLastErrorString(DWORD error);
static inline std::string GetLastErrorString() { return GetLastErrorString(GetLastError()); }

class Win32Exception : public std::runtime_error
{
	DWORD _code;
public:
	inline Win32Exception(DWORD code) : _code(code), std::runtime_error(GetLastErrorString(code)) {}
	inline Win32Exception(DWORD code, const char* operation) : _code(code), std::runtime_error(operation ? (std::string(operation) + " failed: " + GetLastErrorString(code)) : GetLastErrorString(code)) {}
};

static inline void ThrowLastError(const char* operation = nullptr) { DWORD last_error = GetLastError(); throw Win32Exception(last_error, operation); }
static inline void CheckLastError(const char* operation = nullptr) { DWORD last_error = GetLastError(); if (last_error) throw Win32Exception(last_error, operation); }
template<typename T> static inline T CheckLastError(T&& result, const char* operation = nullptr) { CheckLastError(); return std::move(result); }
template<typename T> static inline T ThrowLastErrorIfZero(T&& result, const char* operation = nullptr) { if (result == 0) ThrowLastError(operation); return std::move(result); }
template<typename T> static inline T ThrowLastErrorIfNonZero(T&& result, const char* operation = nullptr) { if (result != 0) ThrowLastError(operation); return std::move(result); }
template<typename T> static inline T ThrowLastErrorIfNegative(T&& result, const char* operation = nullptr) { if (result < 0) ThrowLastError(operation); return std::move(result); }
template<typename T> static inline T ThrowLastErrorIfPositive(T&& result, const char* operation = nullptr) { if (result > 0) ThrowLastError(operation); return std::move(result); }
template<typename T> static inline T ThrowLastErrorIfNull(T&& result, const char* operation = nullptr) { if (result == nullptr) ThrowLastError(operation); return std::move(result); }
template<typename T> static inline T ThrowLastErrorIfNonNull(T&& result, const char* operation = nullptr) { if (result != nullptr) ThrowLastError(operation); return std::move(result); }
static inline BOOL ThrowLastErrorIfFalse(BOOL result, const char* operation = nullptr) { if (!result) ThrowLastError(operation); return result; }
static inline BOOL ThrowLastErrorIfTrue(BOOL result, const char* operation = nullptr) { if (result) ThrowLastError(operation); return result; }
static inline HANDLE ThrowLastErrorIfInvalid(HANDLE result, const char* operation = nullptr) { if (result == INVALID_HANDLE_VALUE) ThrowLastError(operation); return result; }
static inline DWORD CheckError(DWORD result, const char* operation = nullptr) { if (result) throw Win32Exception(result, operation); return result; }

#define WIN_CHECK_IF(predicate, api, args) ThrowLastErrorIf##predicate(api args, #api)
#define WIN_CHECK_IF_ZERO(api, args) ThrowLastErrorIfZero(api args, #api)
#define WIN_CHECK_IF_NONZERO(api, args) ThrowLastErrorIfNonZero(api args, #api)
#define WIN_CHECK_IF_NEGATIVE(api, args) ThrowLastErrorIfNegative(api args, #api)
#define WIN_CHECK_IF_POSITIVE(api, args) ThrowLastErrorIfPositive(api args, #api)
#define WIN_CHECK_IF_NULL(api, args) ThrowLastErrorIfNull(api args, #api)
#define WIN_CHECK_IF_NONNULL(api, args) ThrowLastErrorIfNonNull(api args, #api)
#define WIN_CHECK_IF_FALSE(api, args) ThrowLastErrorIfFalse(api args, #api)
#define WIN_CHECK_IF_TRUE(api, args) ThrowLastErrorIfTrue(api args, #api)
#define WIN_CHECK_IF_INVALID(api, args) ThrowLastErrorIfInvalid(api args, #api)
#define WIN_CHECK_ALWAYS(api, args) CheckLastError(api args, #api)
#define WIN_CHECK_CODE(api, args) CheckError(api args, #api)

#define PrintError(operation, error) LOG(ERROR) << #operation " failed: " << Error(error)
#define PrintLastError(operation) PLOG(ERROR) << #operation " failed: " << LastError

namespace std {
#ifdef UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
#endif
}

template<typename destT, typename srcT> std::basic_string<destT> convert(const srcT* begin, const srcT* const end);
template<> std::basic_string<wchar_t> convert<>(const char* begin, const char* const end);
template<> std::basic_string<char> convert<>(const wchar_t* wbegin, const wchar_t* const wend);

template<typename destT, typename srcT> static inline std::basic_string<destT> convert(const srcT* str)
{
	if (!str || !*str)
		return std::basic_string<destT>();
	return convert<destT>(str, str + std::char_traits<srcT>::length(str));
}
template<> static inline std::basic_string<char> convert<>(const char* str) { return std::basic_string<char>(str); }
template<> static inline std::basic_string<wchar_t> convert<>(const wchar_t* str) { return std::basic_string<wchar_t>(str); }
template<typename destT, typename srcT> static inline std::basic_string<destT> convert(const std::basic_string<srcT>& str)
{
	return convert<destT>(&str[0], &str[0] + str.size());
}
template<> static inline std::basic_string<char> convert<>(const std::basic_string<char>& str) { return str; }
template<> static inline std::basic_string<wchar_t> convert<>(const std::basic_string<wchar_t>& str) { return str; }
template<typename destT, typename srcT> static inline std::basic_string<destT> convert(std::basic_string<srcT>&& str)
{
	return convert<destT>(&str[0], &str[0] + str.size());
}
template<> static inline std::basic_string<char> convert<>(std::basic_string<char>&& str) { return std::move(str); }
template<> static inline std::basic_string<wchar_t> convert<>(std::basic_string<wchar_t>&& str) { return std::move(str); }


std::ostream& operator<<(std::ostream& os, const std::wstring& str);
std::ostream& operator<<(std::ostream& os, const wchar_t* str);
std::wostream& operator<<(std::wostream& os, const std::string& str);
std::wostream& operator<<(std::wostream& os, const char* str);

#ifdef _WIN64
constexpr static inline bool IsWin64() { return true; }
#else
bool IsWin64();
#endif

struct win_tap_adapter
{
	std::string guid;
};

std::vector<win_tap_adapter> win_get_tap_adapters();
BOOL win_install_tap_adapter();
BOOL win_uninstall_tap_adapters();

void win_get_ipv4_routing_table();
