#pragma once

#include "../config.h"
#include "../daemon.h"

#include <vector>

#define PrintError(operation, error) printf("%s failed with error 0x%08xlx\n", #operation, error)
#define PrintLastError(operation) PrintError(operation, GetLastError())

struct win_tap_adapter
{
	std::string guid;
};

std::vector<win_tap_adapter> win_get_tap_adapters();
void win_get_ipv4_routing_table();
