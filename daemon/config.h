#pragma once

#ifdef _WIN32

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#define WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifdef _WIN64
#define WIN64
#endif

#endif

#define THREADSAFE_LOGGING

#define ASIO_STANDALONE
#define ASIO_NO_TYPEID
#define ASIO_MSVC _MSC_VER
#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB

#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <vcruntime.h>
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
#define _WEBSOCKETPP_MOVE_SEMANTICS_
#else
#define _WEBSOCKETPP_CPP11_STL_
#endif

