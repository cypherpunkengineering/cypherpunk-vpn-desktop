#pragma once

#include "logger.h"
#include "path.h"

#include <cstdio>
#include <ctime>

class FileLogger : public Logger
{
protected:
	FILE* _file;
	std::string _name;
	size_t _size, _maxsize;
	bool _timestamps;
public:
	FileLogger() : _file(NULL), _size(0), _maxsize(0), _timestamps(false) {}
	FileLogger(FILE* file) : _file(file), _size(0), _maxsize(0), _timestamps(false) {}

	bool Open(FILE* file)
	{
		_file = file;
		_size = 0;
		_maxsize = 0;
		_timestamps = false;
		return _file != NULL;
	}
	bool Open(std::string filename, size_t maxsize = 10*1024*1024)
	{
		_name = std::move(filename);
		_file = daemon_fopen(_name.c_str(), "a+");
		_size = _file ? (fseek(_file, 0, SEEK_END), ftell(_file)) : 0;
		_maxsize = maxsize;
		_timestamps = true;
		return _file != NULL;
	}

	void Close()
	{
		if (_file)
		{
			daemon_fclose(_file);
			_file = NULL;
		}
	}

	void Archive()
	{
		if (_file)
		{
			if (char* buf = new char[1024*1024])
			{
				if (FILE* f = daemon_fopen((_name + ".old").c_str(), "w"))
				{
					size_t s = (fseek(_file, 0, SEEK_END), ftell(_file));
					fseek(_file, 0, SEEK_SET);

					while (s > 0)
					{
						size_t b = s > 1024 * 1024 ? 1024 * 1024 : s;
						if (!fread(buf, b, 1, _file)) break;
						if (!fwrite(buf, b, 1, f)) break;
						s -= b;
					}
					if (!s)
					{
						fflush(f);
						fseek(_file, 0, SEEK_SET);
						_file = freopen(_name.c_str(), "w+", _file); // truncate current file
						if (_file) fflush(_file);
					}
					fclose(f);
				}
				delete[] buf;
			}
		}
	}

	void Count(size_t chars)
	{
		_size += chars;
		if (_maxsize && _size > _maxsize)
		{
			Archive();
			_size = 0;
		}
	}

	virtual void DoWrite(LogLevel level, const char* fmt, va_list args) override
	{
		if (_file)
		{
			WriteTimestampIfNeeded();
			int size = vfprintf(_file, fmt, args);
			fputc('\n', _file);
			fflush(_file);
			Count(size > 0 ? size + 1 : 1);
		}
		Logger::DoWrite(level, fmt, args);
	}

	virtual void DoWrite(const std::string& str) override
	{
		if (_file)
		{
			WriteTimestampIfNeeded();
			size_t chars = fwrite(str.data(), 1, str.size(), _file);
			fputc('\n', _file);
			fflush(_file);
			Count(chars + 1);
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
