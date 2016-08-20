#include "logger.h"

#ifdef _WIN32
#include "win/win.h"
#include <Windows.h>
#else
#include <cerrno>
#endif

static Logger g_null_logger;
Logger* g_logger = &g_null_logger;
LogLevelMask Logger::levels = STATIC_LOG_LEVELS;
#ifdef THREADSAFE_LOGGING
std::recursive_mutex Logger::_mutex;
#endif
#ifdef _WIN32
const LastErrorWrapper LastError;
#endif

const char* GetBaseName(const char* path)
{
	const char* name = path;
	for (const char* p = name; *p; p++)
		if (*p == '/' || *p == '\\')
			name = p + 1;
	return name;
}

LogWriter& LogWriter::operator<<(LogLevel level)
{
	_str << '[' << Logger::GetLevelString(level) << ']';
	return *this;
}

LogWriter& LogWriter::operator<<(const Location& location)
{
	if (location.func)
	{
		_str << '[' << location.func << ']';
	}
	if (location.file)
	{
		const char* name = location.file;
		for (const char* p = name; *p; p++)
			if (*p == '/' || *p == '\\')
				name = p + 1;
		if (*name)
		{
			_str << '[' << name << ':' << location.line << ']';
		}
	}
	return *this;
}

LogWriter::~LogWriter()
{
	g_logger->Write(_str.str());
}

#ifndef _WIN32
std::string GetLastErrorString(Error::Code code)
{
	return std::string();
}
#endif

std::ostream& operator<<(std::ostream& os, const Error& error)
{
#ifdef _WIN32
	extern std::string GetLastErrorString(DWORD error);
#endif
	std::ios::fmtflags f(os.flags());
	os << GetLastErrorString(error.code) << " (" << std::hex << std::showbase << error.code << ')';
	os.flags(f);
	return os;
}

Error Error::Get()
{
#ifdef _WIN32
	return GetLastError();
#else
	return errno;
#endif
}

void Logger::Push(Logger* logger)
{
	if (logger)
	{
		LOG_LOCK();
		logger->_next = g_logger;
		g_logger = logger;
	}
}

Logger* Logger::Pop()
{
	LOG_LOCK();
	if (g_logger->_next)
	{
		Logger* result = g_logger;
		g_logger = result->_next;
		result->_next = nullptr;
		return result;
	}
	return nullptr;
}
