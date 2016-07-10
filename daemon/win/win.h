#pragma once

#include "config.h"
#include "daemon.h"
#include "logger.h"

#include <vector>
#include <string>

#define PrintError(operation, error) ELOG << LastError(#operation, error)
#define PrintLastError(operation) ELOG << LastError(#operation)

std::string GetLastErrorString(DWORD error);
static inline std::string GetLastErrorString() { return GetLastErrorString(GetLastError()); }
class LastError
{
	const char* _op;
	DWORD _error;
public:
	LastError(const char* op, DWORD error) : _op(op), _error(error) {}
	LastError(const char* op) : LastError(op, GetLastError()) {}

	friend std::ostream& operator<<(std::ostream& os, const LastError& err);
};


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
void win_get_ipv4_routing_table();
