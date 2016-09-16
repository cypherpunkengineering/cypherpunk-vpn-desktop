#pragma once

#include <system_error>

#define THROW_POSIXEXCEPTION(code, api) throw std::system_error(code, std::system_category(), #api " failed")
