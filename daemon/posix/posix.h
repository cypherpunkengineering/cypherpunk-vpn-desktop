#pragma once

#include "debug.h"
#include "util.h"

class PosixException : public SystemException
{
public:
	using SystemException::SystemException;
};

#define THROW_POSIXEXCEPTION(code, api) throw PosixException(code, #api _D(, CURRENT_LOCATION))
