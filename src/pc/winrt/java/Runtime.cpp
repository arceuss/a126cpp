#include "java/Runtime.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Psapi.h is unavailable on UWP; only total physical memory is queryable here.

Runtime Runtime::instance;

Runtime &Runtime::getRuntime()
{
	return instance;
}

long_t Runtime::maxMemory()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	if (!GlobalMemoryStatusEx(&statex))
		return 1;
	return statex.ullTotalPhys;
}

long_t Runtime::totalMemory()
{
	return 0;
}

long_t Runtime::freeMemory()
{
	return 0;
}
