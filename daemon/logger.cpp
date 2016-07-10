#include "logger.h"

static Logger g_null_logger;
Logger* g_logger = &g_null_logger;
LogLevelMask Logger::levels = STATIC_LOG_LEVELS;
#ifdef THREADSAFE_LOGGING
std::recursive_mutex Logger::_mutex;
#endif

LogWriter::LogWriter(LogLevel level)
{
	_str << '[' << Logger::GetLevelString(level) << "] ";
}

LogWriter::LogWriter(LogLevel level, const char* func, const char* file, unsigned int line)
{
	_str << '[' << Logger::GetLevelString(level) << ']';
	if (func && *func)
	{
		_str << '[' << func << ']';
	}
	if (file && *file)
	{
		const char* name = file;
		for (const char* p = file; *p; p++)
			if (*p == '/' || *p == '\\')
				name = p + 1;
		if (*name)
		{
			_str << '[' << name << ':' << line << ']';
		}
	}
	_str << ' ';
}

LogWriter::~LogWriter()
{
	g_logger->Write(_str.str());
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
