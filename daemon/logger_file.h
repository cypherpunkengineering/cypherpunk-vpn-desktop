#pragma once

#include "logger.h"
#include "path.h"

#include <cstdio>
#include <ctime>

class FileLogger : public Logger
{
protected:
	FILE* _file;
	bool _timestamps;
public:
	FileLogger() : _file(nullptr), _timestamps(false) {}
	FileLogger(FILE* file) : _file(file), _timestamps(false) {}

	bool Open(FILE* file)
	{
		_file = file;
		return _file != nullptr;
	}
	bool Open(const char* filename)
	{
		return _timestamps = Open(daemon_fopen(filename, "at"));
	}
	bool Open(const std::string& filename)
	{
		return Open(filename.c_str());
	}

	void Close()
	{
		if (_file)
		{
			daemon_fclose(_file);
			_file = nullptr;
		}
	}

	virtual void DoWrite(LogLevel level, const char* fmt, va_list args) override
	{
		if (_file)
		{
			WriteTimestampIfNeeded();
			vfprintf(_file, fmt, args);
			fputc('\n', _file);
			fflush(_file);
		}
		Logger::DoWrite(level, fmt, args);
	}

	virtual void DoWrite(const std::string& str) override
	{
		if (_file)
		{
			WriteTimestampIfNeeded();
			fwrite(str.data(), 1, str.size(), _file);
			fputc('\n', _file);
			fflush(_file);
		}
		Logger::DoWrite(str);
	}

protected:
	void WriteTimestampIfNeeded()
	{
		if (_file && _timestamps)
		{
			time_t t = time(0);   // get time now
			struct tm * now = localtime(&t);
			fprintf(_file, "[%04d-%02d-%02d %02d:%02d:%02d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		}
	}
};
