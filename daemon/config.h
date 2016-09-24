#pragma once

#ifdef _WIN32 ///////////////////////////////////////////////// Windows //

#define _WIN32_WINNT _WIN32_WINNT_VISTA

#include <SDKDDKVer.h>

#define WIN32
// Reduce the amount of extra stuff defined by Windows.h
#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS      // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES  // VK_*
#define NOWINMESSAGES      // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES        // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS       // SM_*
#define NOMENUS            // MF_*
#define NOICONS            // IDI_*
#define NOKEYSTATES        // MK_*
#define NOSYSCOMMANDS      // SC_*
#define NORASTEROPS        // Binary and Tertiary raster ops
#define NOSHOWWINDOW       // SW_*
#define OEMRESOURCE        // OEM Resource values
#define NOATOM             // Atom Manager routines
#define NOCLIPBOARD        // Clipboard routines
#define NOCOLOR            // Screen colors
#define NOCTLMGR           // Control and Dialog routines
#define NODRAWTEXT         // DrawText() and DT_*
#define NOGDI              // All GDI defines and routines
//#define NOKERNEL           // All KERNEL defines and routines
//#define NOUSER             // All USER defines and routines
//#define NONLS              // All NLS defines and routines
#define NOMB               // MB_* and MessageBox()
#define NOMEMMGR           // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE         // typedef METAFILEPICT
#define NOMINMAX           // Macros min(a,b) and max(a,b)
//#define NOMSG              // typedef MSG and associated routines
#define NOOPENFILE         // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL           // SB_* and scrolling routines
//#define NOSERVICE          // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND            // Sound driver routines
#define NOTEXTMETRIC       // typedef TEXTMETRIC and associated routines
#define NOWH               // SetWindowsHook and WH_*
#define NOWINOFFSETS       // GWL_*, GCL_*, associated routines
#define NOCOMM             // COMM driver routines
#define NOKANJI            // Kanji support stuff.
#define NOHELP             // Help engine interface.
#define NOPROFILER         // Profiler interface.
#define NODEFERWINDOWPOS   // DeferWindowPos routines
#define NOMCX              // Modem Configuration Extensions

#ifdef _WIN64
#define WIN64
#define OS_WIN 64
#else
#define OS_WIN 32
#endif

#elif __APPLE__ ////////////////////////////////////////////////// OS X //

#define OS_OSX 1

#include <sys/param.h>

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

#if OS_LINUX
#include <sys/types.h>
#endif

namespace rapidjson { typedef size_t SizeType; }
