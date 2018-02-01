#pragma once

#include "config.h"

#include "debug.h"
#include "util.h"

class PosixException : public SystemException
{
public:
	using SystemException::SystemException;
};

#define THROW_POSIXEXCEPTION(code, api) throw PosixException(code, #api _D(, CURRENT_LOCATION))

                     static inline void   ThrowLastError          (               const char* operation _D(, LOCATION_DECL)) { int e = errno;   throw PosixException(e, operation _D(, LOCATION_VAR)); }
template<typename T> static inline T      ThrowLastErrorIfMinusOne(T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result == -1)      ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfZero    (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result == 0)       ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNonZero (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result != 0)       ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNegative(T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result < 0)        ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfPositive(T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result > 0)        ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNull    (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result == nullptr) ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
template<typename T> static inline T      ThrowLastErrorIfNonNull (T&&    result, const char* operation _D(, LOCATION_DECL)) { if (result != nullptr) ThrowLastError(   operation _D(, LOCATION_VAR)); return std::move(result); }
                     static inline bool   ThrowLastErrorIfFalse   (bool   result, const char* operation _D(, LOCATION_DECL)) { if (result) {} else    ThrowLastError(   operation _D(, LOCATION_VAR)); return result; }
                     static inline bool   ThrowLastErrorIfTrue    (bool   result, const char* operation _D(, LOCATION_DECL)) { if (result)            ThrowLastError(   operation _D(, LOCATION_VAR)); return result; }

#define POSIX_CHECK_IF(predicate, api, args) ThrowLastErrorIf##predicate(api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_ZERO(api, args)       ThrowLastErrorIfZero       (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_NONZERO(api, args)    ThrowLastErrorIfNonZero    (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_NEGATIVE(api, args)   ThrowLastErrorIfNegative   (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_POSITIVE(api, args)   ThrowLastErrorIfPositive   (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_NULL(api, args)       ThrowLastErrorIfNull       (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_NONNULL(api, args)    ThrowLastErrorIfNonNull    (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_FALSE(api, args)      ThrowLastErrorIfFalse      (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK_IF_TRUE(api, args)       ThrowLastErrorIfTrue       (api args, #api _D(, CURRENT_LOCATION))
#define POSIX_CHECK(api, args)               ThrowLastErrorIfMinusOne   (api args, #api _D(, CURRENT_LOCATION))
