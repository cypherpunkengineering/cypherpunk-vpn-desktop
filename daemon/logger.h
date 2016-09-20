#pragma once

#include "config.h"
#include "debug.h"
#include "util.h"

#include <cstdarg>
#include <sstream>
#ifdef THREADSAFE_LOGGING
#include <mutex>
#endif

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

#define LOG_ENABLED(level) (STATIC_LOG_ENABLED(level) && (Logger::levels & (1 << (level))))

#define Log( level, fmt, ...) g_logger->Write(level, fmt,##__VA_ARGS__)
#define LogCritical(fmt, ...) g_logger->Write(LEVEL_CRITICAL, fmt,##__VA_ARGS__)
#define LogError(   fmt, ...) g_logger->Write(LEVEL_ERROR,    fmt,##__VA_ARGS__)
#define LogWarning( fmt, ...) g_logger->Write(LEVEL_WARNING,  fmt,##__VA_ARGS__)
#define LogInfo(    fmt, ...) g_logger->Write(LEVEL_INFO,     fmt,##__VA_ARGS__)
#define LogVerbose( fmt, ...) g_logger->Write(LEVEL_VERBOSE,  fmt,##__VA_ARGS__)

#ifdef _DEBUG
#define LOG_WITH_LOCATION (Location(__func__, __FILE__, __LINE__))
#else
#define LOG_WITH_LOCATION
#endif

#define LOG_EX(classname, level, condition, ...) (!(LOG_ENABLED(level) && (condition))) ? (void)0 : Voidify() | classname(__VA_ARGS__)(level) LOG_WITH_LOCATION

#define LOG_IF(severity, condition)  LOG_EX(PrefixLogWriter<LogWriter>, LEVEL_##severity, condition)
#define LOG(severity)                LOG_EX(PrefixLogWriter<LogWriter>, LEVEL_##severity, true)
#define PLOG_IF(severity, condition) LOG_EX(PrefixLogWriter<ErrorLogWriter>, LEVEL_##severity, condition, Error::Get())
#define PLOG(severity)               LOG_EX(PrefixLogWriter<ErrorLogWriter>, LEVEL_##severity, true, Error::Get())


struct Voidify
{
	template<typename T> inline void operator|(T&&) const {}
};

class Error
{
public:
#ifdef _WIN32
	typedef unsigned long Code;
#else
	typedef int Code;
#endif
	Code code;
public:
	constexpr Error(Code code) : code(code) {}
	static Error Get();

	friend std::ostream& operator<<(std::ostream& os, const Error& error);
};
class LastErrorWrapper {};
extern const LastErrorWrapper LastError;

class LogWriter
{
protected:
	friend class Logger;
	std::ostringstream _str;

public:
	~LogWriter();

	template<typename T>
	inline LogWriter& operator<<(T&& arg) { _str << std::forward<T>(arg); return *this; }
	LogWriter& operator<<(LogLevel level);
	LogWriter& operator<<(const Location& location);
	LogWriter& operator<<(const class LastErrorWrapper& error) { _str << Error::Get(); return *this; }
};

template<class WRITER>
class PrefixLogWriter
{
	WRITER _writer;
public:
	template<typename... Args>
	PrefixLogWriter(Args&&... args) : _writer(std::forward<Args>(args)...) {}

	inline PrefixLogWriter& operator()() { return *this; }
	PrefixLogWriter& operator()(LogLevel level) { _writer << level; return *this; }
	PrefixLogWriter& operator()(const Location& location) { _writer << location; return *this; }

	template<typename T>
	inline WRITER& operator<<(T&& arg) { return _writer << ' ' << std::forward<T>(arg); }
};

class ErrorLogWriter : public LogWriter
{
	Error _error;

public:
	ErrorLogWriter(Error error) : _error(error) {}

	template<typename T>
	inline ErrorLogWriter& operator<<(T&& arg) { *(LogWriter*)this << std::forward<T>(arg); return *this; }
	ErrorLogWriter& operator<<(const class LastErrorWrapper& error) { *(LogWriter*)this << _error; return *this; }
};

class Logger
{
private:
	Logger* _next;

public:
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
			(level == LEVEL_ERROR) ? "ERROR" :
			(level == LEVEL_WARNING) ? "WARNING" :
			(level == LEVEL_INFO) ? "INFO" :
			(level == LEVEL_VERBOSE) ? "VERBOSE" :
			"";
	}
};

const char* GetBaseName(const char* path);
