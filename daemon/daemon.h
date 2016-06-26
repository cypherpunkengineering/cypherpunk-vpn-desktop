#pragma once

#include "config.h"

extern class CypherDaemon* g_daemon;

enum LogLevel
{
	LEVEL_CRITICAL,
	LEVEL_ERROR,
	LEVEL_WARNING,
	LEVEL_INFO,
};

#define LogCritical(fmt, ...) g_daemon->Log(LEVEL_CRITICAL, fmt, ## __VA_ARGS__)
#define LogError(   fmt, ...) g_daemon->Log(LEVEL_ERROR,    fmt, ## __VA_ARGS__)
#define LogWarning( fmt, ...) g_daemon->Log(LEVEL_WARNING,  fmt, ## __VA_ARGS__)
#define LogInfo(    fmt, ...) g_daemon->Log(LEVEL_INFO,     fmt, ## __VA_ARGS__)


class CypherDaemon
{
public:
	CypherDaemon();

public:
	// The main entrypoint of the daemon; should block for the duration of the daemon.
	int Run();

	// Requests that the daemon begin shutting down. Can be called either from within
	// the daemon or from another thread.
	void RequestShutdown();

public:
	virtual void Log(LogLevel level, const char* fmt, ...) {}
};

