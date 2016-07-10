#pragma once

#include "config.h"

enum LogLevel
{
	LEVEL_CRITICAL,
	LEVEL_ERROR,
	LEVEL_WARNING,
	LEVEL_INFO,
	LEVEL_VERBOSE,
};
typedef unsigned int LogLevelMask;

extern class Logger* g_logger;

#define LEVEL_ALL ((LogLevelMask)0xFFFFFFFU)

#define STATIC_LOG_LEVELS LEVEL_ALL
#define STATIC_LOG_ENABLED(level) (STATIC_LOG_LEVELS & (1 << (level)))

#define LOG_ENABLED(level) (Logger::levels & (1 << (level)))

#define Log( level, fmt, ...) g_logger->Write(level, fmt,##__VA_ARGS__)
#define LogCritical(fmt, ...) g_logger->Write(LEVEL_CRITICAL, fmt,##__VA_ARGS__)
#define LogError(   fmt, ...) g_logger->Write(LEVEL_ERROR,    fmt,##__VA_ARGS__)
#define LogWarning( fmt, ...) g_logger->Write(LEVEL_WARNING,  fmt,##__VA_ARGS__)
#define LogInfo(    fmt, ...) g_logger->Write(LEVEL_INFO,     fmt,##__VA_ARGS__)
#define LogVerbose( fmt, ...) g_logger->Write(LEVEL_VERBOSE,  fmt,##__VA_ARGS__)

#ifdef _DEBUG
#define LOGWRITER(level) LogWriter(level, __func__, __FILE__, __LINE__)
#else
#define LOGWRITER(level) LogWriter(level)
#endif

#define LOG(level) (!(STATIC_LOG_ENABLED(level) && LOG_ENABLED(level))) ? (void)0 : Voidify() | LOGWRITER(level)
#define CLOG LOG(LEVEL_CRITICAL)
#define WLOG LOG(LEVEL_ERROR)
#define ELOG LOG(LEVEL_WARNING)
#define ILOG LOG(LEVEL_INFO)
#define VLOG LOG(LEVEL_VERBOSE)

#include <cstdarg>
#include <sstream>
#ifdef THREADSAFE_LOGGING
#include <mutex>
#endif


struct Voidify
{
	template<typename T> inline void operator|(T&&) const {}
};

class LogWriter
{
	friend class Logger;
	std::ostringstream _str;

public:
	LogWriter(LogLevel level);
	LogWriter(LogLevel level, const char* func, const char* file = nullptr, unsigned int line = 0);
	~LogWriter();

	template<typename T>
	inline LogWriter& operator<<(T&& arg)
	{
		_str << std::forward<T>(arg);
		return *this;
	}
};

class Logger
{
private:
	Logger* _next;

#ifdef THREADSAFE_LOGGING
	static std::recursive_mutex _mutex;
#define LOG_LOCK() std::lock_guard<std::recursive_mutex> lock(_mutex)
#else
#define LOG_LOCK() ((void)0)
#endif

public:
	static LogLevelMask levels;
public:
	Logger() : _next(nullptr) {}

	inline void Write(LogLevel level, const char* fmt, ...)
	{
		LOG_LOCK();
		va_list args;
		va_start(args, fmt);
		DoWrite(level, fmt, args);
		va_end(args);
	}
	inline void Write(LogLevel level, const char* fmt, va_list args)
	{
		LOG_LOCK();
		DoWrite(level, fmt, args);
	}
	inline void Write(const std::string& str)
	{
		LOG_LOCK();
		DoWrite(str);
	}
protected:
	virtual void DoWrite(LogLevel level, const char* fmt, va_list args)
	{
		if (_next) _next->DoWrite(level, fmt, args);
	}
	virtual void DoWrite(const std::string& str)
	{
		if (_next) _next->DoWrite(str);
	}
public:
	static void Push(Logger* logger);
	static Logger* Pop();
	static inline void Enable(LogLevel level)
	{
		levels |= (1 << level);
	}
	static inline void Disable(LogLevel level)
	{
		levels &= ~(1 << level);
	}
public:
	static constexpr const char* GetLevelString(LogLevel level)
	{
		return
			(level == LEVEL_CRITICAL) ? "CRITICAL" :
			(level == LEVEL_ERROR   ) ? "ERROR"    :
			(level == LEVEL_WARNING ) ? "WARNING"  :
			(level == LEVEL_INFO    ) ? "INFO"     :
			(level == LEVEL_VERBOSE ) ? "VERBOSE"  :
			"";
	}
};
