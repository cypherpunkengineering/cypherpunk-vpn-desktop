// cypherpunk-desktop-daemon.cpp : Defines the entry point for the console application.
//

#include "config.h"

#include "daemon.h"

#include <thread>
#include <asio.hpp>

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

CypherDaemon* g_daemon = nullptr;

#include <stdio.h>
#include "win/win.h"


CypherDaemon::CypherDaemon()
{
	extern char HEX[256][4];
	for (int i = 0; i < 256; i++)
		sprintf_s(HEX[i], "%02x", i);
}

int CypherDaemon::Run()
{
	auto taps = win_get_tap_adapters();
	win_get_ipv4_routing_table();
	return 0;
}

void CypherDaemon::RequestShutdown()
{

}
