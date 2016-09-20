#pragma once

#include "config.h"

#include <string>

class NullLocation
{
public:
	constexpr NullLocation(const char* func, const char* file, int line) {}
	constexpr NullLocation(const char* file, int line) {}
	constexpr NullLocation(const char* func) {}
	constexpr NullLocation() {}
	constexpr const char* func() const { return nullptr; }
	constexpr const char* path() const { return nullptr; }
	constexpr const char* file() const { return nullptr; }
	constexpr int line() const { return 0; }
};

class Location
{
	const char* _func;
	const char* _file;
	int _line;
public:
	constexpr Location(const char* func, const char* file, int line) : _func(func), _file(file), _line(line) {}
	constexpr Location(const char* file, int line) : _func(nullptr), _file(file), _line(line) {}
	constexpr Location(const char* func) : _func(func), _file(nullptr), _line(0) {}
	constexpr Location() : _func(nullptr), _file(nullptr), _line(0) {}
	const char* func() const { return _func; }
	const char* path() const { return _file; }
	const char* file() const
	{
		if (!_file) return nullptr;
		const char* p = _file;
		const char* begin = p;
		while (*p)
		{
			if (*p == '/' || *p == '\\')
				begin = ++p;
			else
				++p;
		}
		return begin;
	}
	int line() const { return _line; }
	std::string where() const
	{
		if (_file)
			return std::string(file()) + ":" + std::to_string(_line);
		else
			return std::string("<unknown>");
	}
};

//constexpr const Location NullLocation;

#define CURRENT_LOCATION Location(__func__, __FILE__, __LINE__)
#define GLOBAL_LOCATION Location(__FILE__, __LINE__)
#define LOCATION_VAR _caller_location
#define LOCATION_DECL const Location& LOCATION_VAR

#ifdef _DEBUG
# define _D(...) __VA_ARGS__
# define _R(...)
	typedef Location DebugLocation;
#else
# define _D(...)
# define _R(...) __VA_ARGS__
	typedef NullLocation DebugLocation;
#endif

