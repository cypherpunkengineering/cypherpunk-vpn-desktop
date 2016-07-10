#pragma once

#include "logger.h"

#include <cstdio>

class FileLogger : public Logger
{
protected:
	FILE* _file;
public:
	FileLogger() : _file(nullptr) {}
	FileLogger(FILE* file) : _file(file) {}

	bool Open(FILE* file)
	{
		_file = file;
		return _file != nullptr;
	}
	bool Open(const char* filename)
	{
		return Open(fopen(filename, "at"));
	}
	bool Open(const std::string& filename)
	{
		return Open(filename.c_str());
	}

	void Close()
	{
		if (_file)
		{
			fclose(_file);
			_file = nullptr;
		}
	}

	virtual void DoWrite(LogLevel level, const char* fmt, va_list args) override
	{
		if (_file)
		{
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
			fwrite(str.data(), 1, str.size(), _file);
			fputc('\n', _file);
			fflush(_file);
		}
		Logger::DoWrite(str);
	}
};
