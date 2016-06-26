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

#define ASIO_STANDALONE
#define ASIO_NO_TYPEID
#define ASIO_MSVC _MSC_VER
#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB

