#pragma once

#include <string>

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <cstdio>
#endif

namespace CrashMessageBox
{
	inline void showError(const char *title, const std::string &text)
	{
#ifdef _WIN32
		MessageBoxA(NULL, text.c_str(), title ? title : "Error", MB_OK | MB_ICONERROR);
#else
		if (title && *title)
			std::fprintf(stderr, "%s: %s\n", title, text.c_str());
		else
			std::fprintf(stderr, "%s\n", text.c_str());
#endif
	}
}
