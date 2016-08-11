#pragma once

#ifdef _WIN32 ///////////////////////////////////////////////// Windows //

#include <SDKDDKVer.h>

#define WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifdef _WIN64
#define WIN64
#define OS_WIN 64
#else
#define OS_WIN 32
#endif

#elif __APPLE__ ////////////////////////////////////////////////// OS X //

#define OS_OSX 1

#elif __linux__ ///////////////////////////////////////////////// Linux //

#define OS_LINUX 1

#else ////////////////////////////////////////////////////// Unknown OS //

#error "Unknown OS"

#endif ///////////////////////////////////////////////////////////////////



#define THREADSAFE_LOGGING

#define ASIO_STANDALONE
#define ASIO_NO_TYPEID
#define ASIO_MSVC _MSC_VER
#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB

#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NO_SIZETYPEDEFINE
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#ifdef _MSC_VER ///////////////////////////////////////// Visual Studio //

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <vcruntime.h>
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
#define _WEBSOCKETPP_MOVE_SEMANTICS_

#else ////////////////////////////////////////////////////////////////////

#define _WEBSOCKETPP_CPP11_STL_

#endif ///////////////////////////////////////////////////////////////////


namespace rapidjson { typedef size_t SizeType; }
