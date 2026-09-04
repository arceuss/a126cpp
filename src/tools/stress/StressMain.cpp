#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif


#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#ifdef __SWITCH__
#include "pc/switch/SwitchRuntime.h"
#endif
#include "tools/stress/StressHarness.h"

#include "backends/Backend.h"
#include "client/renderer/culling/OcclusionCuller.h"
#include "legacygl/Context.h"
#include "lwjgl/GLContext.h"
#ifdef __SWITCH__
// libnx calls userAppInit before the C++ static constructors and userAppExit
// after the destructors, which is the only correct place for romfs: the game
// has static objects that read resources.
extern "C" void userAppInit(void)
{
	switchruntime::initialize();
}

extern "C" void userAppExit(void)
{
	switchruntime::shutdown();
}
#endif

#if defined(__SWITCH__) && defined(A126_ENABLE_MEMORY_PROBE)
// Homebrew launched from hbmenu gets no argv, so the harness reads the same
// whitespace-separated command line as the client profiling build. Absent
// file: usage error, since a stress run without a scenario is meaningless.
static std::vector<std::string> readArgumentFile()
{
	std::vector<std::string> arguments;
	FILE *file = std::fopen("sdmc:/switch/a126cpp-args.txt", "r");
	if (file == nullptr)
		return arguments;
	std::string content;
	char chunk[256];
	std::size_t read = 0;
	while ((read = std::fread(chunk, 1, sizeof(chunk), file)) > 0)
		content.append(chunk, read);
	std::fclose(file);
	std::istringstream stream(content);
	std::string token;
	while (stream >> token)
		arguments.push_back(token);
	if (!arguments.empty())
		std::fprintf(stderr, "switch: %zu arguments read from a126cpp-args.txt\n", arguments.size());
	return arguments;
}
#endif


// A stress run that dies silently is a lost finding. These handlers name the
// exception or fault and print a symbolized stack on the way out.
#if defined(_WIN32)
static void printStack(CONTEXT *context)
{
	HANDLE process = GetCurrentProcess();
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
	SymInitialize(process, nullptr, TRUE);
	void *frames[64];
	USHORT count = 0;
	if (context == nullptr)
	{
		count = CaptureStackBackTrace(1, 64, frames, nullptr);
	}
	else
	{
		STACKFRAME64 frame = {};
		frame.AddrPC.Offset = context->Rip;
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrFrame.Offset = context->Rbp;
		frame.AddrFrame.Mode = AddrModeFlat;
		frame.AddrStack.Offset = context->Rsp;
		frame.AddrStack.Mode = AddrModeFlat;
		CONTEXT copy = *context;
		while (count < 64 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(),
			&frame, &copy, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
		{
			if (frame.AddrPC.Offset == 0)
				break;
			frames[count++] = reinterpret_cast<void *>(frame.AddrPC.Offset);
		}
	}
	alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256];
	SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(buffer);
	for (USHORT i = 0; i < count; i++)
	{
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = 255;
		DWORD64 displacement = 0;
		IMAGEHLP_LINE64 line = {};
		line.SizeOfStruct = sizeof(line);
		DWORD lineDisplacement = 0;
		const DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
		if (SymFromAddr(process, address, &displacement, symbol))
		{
			if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
				std::fprintf(stderr, "  %s+0x%llx  %s:%lu\n", symbol->Name,
					static_cast<unsigned long long>(displacement), line.FileName, line.LineNumber);
			else
				std::fprintf(stderr, "  %s+0x%llx\n", symbol->Name, static_cast<unsigned long long>(displacement));
		}
		else
		{
			std::fprintf(stderr, "  0x%llx\n", static_cast<unsigned long long>(address));
		}
	}
	std::fflush(stderr);
}

static LONG WINAPI onUnhandledException(EXCEPTION_POINTERS *info)
{
	std::fprintf(stderr, "stress: fatal exception 0x%08lx at 0x%llx\n",
		info->ExceptionRecord->ExceptionCode,
		static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(info->ExceptionRecord->ExceptionAddress)));
	printStack(info->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void onTerminate()
{
	std::exception_ptr current = std::current_exception();
	if (current)
	{
		try { std::rethrow_exception(current); }
		catch (const std::exception &e) { std::fprintf(stderr, "stress: terminate: %s\n", e.what()); }
		catch (...) { std::fprintf(stderr, "stress: terminate: non-std exception\n"); }
	}
	else
	{
		std::fprintf(stderr, "stress: terminate without an active exception\n");
	}
#if defined(_WIN32)
	printStack(nullptr);
#endif
	std::fflush(stderr);
	std::abort();
}

// Usage:
//   a126cpp-stress [--backend <name>] [--null-sink] [--frames N]
//                  [--tick-interval N] [--warmup N] [--sample-every N]
//                  [--view-distance 0-3] [--no-finish] [--log <path>]
//                  [--server host[:port]] [--user name] [--world <save>]
//                  <scenario> [--<param> <value>]...
//
// --backend must come first, as in the client. --null-sink keeps that
// backend's window but routes every draw to legacygl's resident null sink,
// which is the measurement of everything above the backend boundary on the
// same core path a packet backend takes. --server runs the scenario on a
// live server's level instead of a generated one.

static void usage()
{
	std::fprintf(stderr,
		"usage: a126cpp-stress [--backend <name>] [--null-sink] [--frames N]\n"
		"                      [--tick-interval N] [--warmup N] [--sample-every N]\n"
		"                      [--view-distance 0-3] [--no-finish] [--log <path>]\n"
		"                      [--server host[:port]] [--user name] [--world <save>]\n"
		"                      <scenario|all> [--<param> <value>]...\n"
		"all runs every offline scenario in one process (one log per scenario).\n"
		"scenarios:");
	for (const std::string &name : stress::scenarioNames())
		std::fprintf(stderr, " %s", name.c_str());
	std::fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
	std::set_terminate(&onTerminate);
	stress::Options options;
	int i = 1;
#if defined(__SWITCH__) && defined(A126_ENABLE_MEMORY_PROBE)
	const std::vector<std::string> fileArguments = readArgumentFile();
	std::vector<char *> fileArgv;
	if (!fileArguments.empty())
	{
		fileArgv.push_back(argc > 0 ? argv[0] : const_cast<char *>("a126cpp-stress"));
		for (const std::string &argument : fileArguments)
			fileArgv.push_back(const_cast<char *>(argument.c_str()));
		argc = static_cast<int>(fileArgv.size());
		argv = fileArgv.data();
	}
#endif
	if (i < argc && std::strcmp(argv[i], "--backend") == 0)
	{
		if (i + 1 >= argc)
		{
			std::fprintf(stderr, "error: --backend requires a name\n");
			return 2;
		}
		if (!renderbackend::select(argv[i + 1]))
		{
			std::fprintf(stderr, "error: unknown or unavailable backend '%s'\n", argv[i + 1]);
			return 2;
		}
		i += 2;
	}

	auto intArg = [&](int &target) -> bool
	{
		if (i + 1 >= argc)
		{
			std::fprintf(stderr, "error: %s requires a value\n", argv[i]);
			return false;
		}
		target = std::atoi(argv[i + 1]);
		i += 2;
		return true;
	};
	auto stringArg = [&](std::string &target) -> bool
	{
		if (i + 1 >= argc)
		{
			std::fprintf(stderr, "error: %s requires a value\n", argv[i]);
			return false;
		}
		target = argv[i + 1];
		i += 2;
		return true;
	};

	// Harness options and scenario parameters may be mixed in any order after
	// --backend; the first bare word is the scenario.
	while (i < argc)
	{
		const char *arg = argv[i];
		if (std::strcmp(arg, "--null-sink") == 0) { options.nullSink = true; i++; }
		else if (std::strcmp(arg, "--no-finish") == 0) { options.finishEachFrame = false; i++; }
		else if (std::strcmp(arg, "--capture") == 0)
		{
			if (!stringArg(options.capturePath)) return 2;
		}
		else if (std::strcmp(arg, "--fancy") == 0) { if (!intArg(options.fancyGraphics)) return 2; }
		else if (std::strcmp(arg, "--no-occlusion") == 0) { OcclusionCuller::enabled = false; i++; }
		else if (std::strcmp(arg, "--frames") == 0) { if (!intArg(options.frames)) return 2; }
		else if (std::strcmp(arg, "--tick-interval") == 0) { if (!intArg(options.tickInterval)) return 2; }
		else if (std::strcmp(arg, "--warmup") == 0) { if (!intArg(options.warmupFrames)) return 2; }
		else if (std::strcmp(arg, "--sample-every") == 0) { if (!intArg(options.sampleEvery)) return 2; }
		else if (std::strcmp(arg, "--view-distance") == 0) { if (!intArg(options.viewDistance)) return 2; }
		else if (std::strcmp(arg, "--log") == 0)
		{
			if (!stringArg(options.logPath)) return 2;
		}
		else if (std::strcmp(arg, "--user") == 0)
		{
			if (!stringArg(options.user)) return 2;
		}
		else if (std::strcmp(arg, "--world") == 0)
		{
			if (!stringArg(options.world)) return 2;
		}
		else if (std::strcmp(arg, "--server") == 0)
		{
			std::string server;
			if (!stringArg(server)) return 2;
			const std::size_t colon = server.rfind(':');
			if (colon != std::string::npos)
			{
				options.serverPort = std::atoi(server.c_str() + colon + 1);
				server.resize(colon);
			}
			options.serverHost = server;
		}
		else if (std::strncmp(arg, "--", 2) == 0)
		{
			if (options.scenario.empty() || i + 1 >= argc)
			{
				std::fprintf(stderr, "error: unknown option '%s'\n", arg);
				usage();
				return 2;
			}
			options.params.values[arg + 2] = argv[i + 1];
			i += 2;
		}
		else if (options.scenario.empty()) { options.scenario = arg; i++; }
		else
		{
			std::fprintf(stderr, "error: unexpected argument '%s'\n", arg);
			usage();
			return 2;
		}
	}
	if (options.scenario.empty())
	{
		usage();
		return 2;
	}
	if (options.tickInterval < 1)
		options.tickInterval = 1;
	if (options.sampleEvery < 1)
		options.sampleEvery = 1;
	if (options.warmupFrames < 0)
		options.warmupFrames = 0;
	if (options.viewDistance < 0 || options.viewDistance > 3)
		options.viewDistance = 0;
	if (options.logPath.empty())
	{
		options.logPath = "stress-" + options.scenario +
			(options.serverHost.empty() ? std::string() : std::string("-mp")) + "-" +
			(options.nullSink ? std::string("null") : std::string(renderbackend::configuration().recordName)) +
			".log";
	}

	// Window, API context, and the selected backend's sink, exactly as the
	// client. The null sink then replaces the backend before any draw is made,
	// so the context keeps its legacy defaults and nothing reaches the GPU.
	lwjgl::GLContext::instantiate();
	if (options.nullSink)
		legacygl::setSink(legacygl::residentNullSink());

	return stress::run(options);
}
