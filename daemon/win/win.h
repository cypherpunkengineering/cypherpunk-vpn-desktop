#pragma once

#include "config.h"
#include "daemon.h"
#include "debug.h"
#include "logger.h"
#include "serialization.h"
#include "util.h"

#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>



//std::string GetLastErrorString(DWORD error);
//static inline std::string GetLastErrorString() { return GetLastErrorString(GetLastError()); }

class Win32Exception : public SystemException
{
public:
	inline Win32Exception(DWORD code, const char* from _D(, const Location& location)) : SystemException(code, from _D(, location)) {}
	inline Win32Exception(DWORD code _D(, const Location& location)) : SystemException(code, nullptr _D(, location)) {}

	DWORD value() const { return code().value(); }
};

#define THROW_WIN32EXCEPTION(code, api) throw Win32Exception(code, #api _D(, CURRENT_LOCATION))

                     static inline void   ThrowLastError          (               const char* operation _D(, LOCATION_DECL)) { DWORD e = GetLastError();           throw Win32Exception(e,      operation _D(, LOCATION_VAR)); }
                     static inline void   CheckLastError          (               const char* operation _D(, LOCATION_DECL)) { DWORD e = GetLastError(); if (e)    throw Win32Exception(e,      operation _D(, LOCATION_VAR)); }
template<typename T> static inline T      CheckLastError          (T&&    result, const char* operation _D(, LOCATION_DECL)) {                                     CheckLastError(                        _D(  LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfZero    (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result == 0)                    ThrowLastError(              operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNonZero (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result != 0)                    ThrowLastError(              operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNegative(T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result < 0)                     ThrowLastError(              operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfPositive(T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result > 0)                     ThrowLastError(              operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNull    (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result == nullptr)              ThrowLastError(              operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNonNull (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result != nullptr)              ThrowLastError(              operation _D(, LOCATION_VAR)); return std::move(result); }
                     static inline BOOL   ThrowLastErrorIfFalse   (BOOL   result, const char* operation _D(, LOCATION_DECL)) { if (!result)                        ThrowLastError(              operation _D(, LOCATION_VAR)); return result; }
                     static inline BOOL   ThrowLastErrorIfTrue    (BOOL   result, const char* operation _D(, LOCATION_DECL)) { if (result)                         ThrowLastError(              operation _D(, LOCATION_VAR)); return result; }
                     static inline HANDLE ThrowLastErrorIfInvalid (HANDLE result, const char* operation _D(, LOCATION_DECL)) { if (result == INVALID_HANDLE_VALUE) ThrowLastError(              operation _D(, LOCATION_VAR)); return result; }
                     static inline DWORD  CheckError              (DWORD  result, const char* operation _D(, LOCATION_DECL)) { if (result != ERROR_SUCCESS)        throw Win32Exception(result, operation _D(, LOCATION_VAR)); return result; }

#define WIN_CHECK_IF(predicate, api, args) ThrowLastErrorIf##predicate(api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_ZERO(api, args)       ThrowLastErrorIfZero       (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_NONZERO(api, args)    ThrowLastErrorIfNonZero    (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_NEGATIVE(api, args)   ThrowLastErrorIfNegative   (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_POSITIVE(api, args)   ThrowLastErrorIfPositive   (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_NULL(api, args)       ThrowLastErrorIfNull       (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_NONNULL(api, args)    ThrowLastErrorIfNonNull    (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_FALSE(api, args)      ThrowLastErrorIfFalse      (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_TRUE(api, args)       ThrowLastErrorIfTrue       (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_IF_INVALID(api, args)    ThrowLastErrorIfInvalid    (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_ALWAYS(api, args)        CheckLastError             (api args, #api _D(, CURRENT_LOCATION))
#define WIN_CHECK_RESULT(api, args)        CheckError                 (api args, #api _D(, CURRENT_LOCATION))

#define PrintError(operation, error) LOG(ERROR) << #operation " failed: " << Error(error)
#define PrintLastError(operation)   PLOG(ERROR) << #operation " failed: " << LastError


struct Win32HandleBase
{
	static BOOL CloseHandle(HANDLE handle) noexcept { return ::CloseHandle(handle); }
};

template<class IMPL = Win32HandleBase>
class TypedWin32Handle : protected noncopyable
{
protected:
	HANDLE _handle;
private:
	bool CloseImpl() noexcept { return _handle == NULL || IMPL::CloseHandle(_handle); }
	TypedWin32Handle(HANDLE handle) : _handle(handle) {}
public:
	TypedWin32Handle() : _handle(NULL) {}
	TypedWin32Handle(TypedWin32Handle&& other) : _handle(std::exchange(other._handle, NULL)) {}
	~TypedWin32Handle() noexcept { CloseImpl(); }

	static TypedWin32Handle Wrap(HANDLE handle) { return TypedWin32Handle(handle); }

	TypedWin32Handle& operator=(TypedWin32Handle&& other) { CloseImpl(); _handle = std::exchange(other._handle, NULL); return *static_cast<DERIVED>(this); }
	operator HANDLE() const { return _handle; }
	operator bool() const { return _handle != NULL; }
	bool operator!() const { return _handle == NULL; }
	HANDLE Release() { return std::exchange(_handle, NULL); }
	void Close() { WIN_CHECK_IF_FALSE(CloseImpl, ()); _handle = NULL; }
};
typedef TypedWin32Handle<> Win32Handle;


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


template<> struct serialization::json_converter<GUID, JsonValue> { static void convert(GUID& dst, const JsonValue& src); };
template<> struct serialization::json_converter<JsonValue, GUID> { static void convert(JsonValue& dst, const GUID& src); };


struct win_tap_adapter
{
	std::string guid;
};

std::vector<win_tap_adapter> win_get_tap_adapters();
BOOL win_install_tap_adapter();
BOOL win_uninstall_tap_adapters();

void win_get_ipv4_routing_table();
