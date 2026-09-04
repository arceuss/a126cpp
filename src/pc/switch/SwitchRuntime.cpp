#include "switch/SwitchRuntime.h"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <sys/stat.h>
#include <typeinfo>

#include <switch.h>

// Userland exception handling, per the libnx exception-handler example. A
// hardware fault on Switch produces no dialog and no output, so without this
// a crash is completely silent. The dump lets the faulting PC be resolved
// against the ELF with aarch64-none-elf-addr2line.
extern "C"
{
alignas(16) u8 __nx_exception_stack[0x1000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

void __libnx_exception_handler(ThreadExceptionDump *ctx)
{
	FILE *dump = std::fopen("sdmc:/switch/a126cpp-crash.txt", "w");
	if (dump == nullptr)
		return;

	std::fprintf(dump, "error_desc: 0x%x\n", ctx->error_desc);
	std::fprintf(dump, "pc:  0x%lx\n", ctx->pc.x);
	std::fprintf(dump, "lr:  0x%lx\n", ctx->lr.x);
	std::fprintf(dump, "sp:  0x%lx\n", ctx->sp.x);
	std::fprintf(dump, "fp:  0x%lx\n", ctx->fp.x);
	std::fprintf(dump, "far: 0x%lx\n", ctx->far.x);
	std::fprintf(dump, "esr: 0x%x\n", ctx->esr);

	// The NRO is position independent, so a runtime PC means nothing on its
	// own. Recording a known symbol's runtime address makes the load base
	// recoverable: base = anchor - <address of switchruntime::shutdown in ELF>.
	std::fprintf(dump, "anchor_shutdown: 0x%lx\n",
		(unsigned long)reinterpret_cast<uintptr_t>(&switchruntime::shutdown));

	for (int i = 0; i < 29; i++)
		std::fprintf(dump, "X%d: 0x%lx\n", i, ctx->cpu_gprs[i].x);

	std::fclose(dump);
}
}

namespace switchruntime
{

static bool networkAvailable = false;

// There is no console and no crash dialog, so an uncaught exception would
// otherwise abort with nothing written anywhere. This turns it into a readable
// line in the log.
static void reportTerminate()
{
	std::fprintf(stderr, "switch: std::terminate called\n");

	if (std::exception_ptr active = std::current_exception())
	{
		try
		{
			std::rethrow_exception(active);
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "switch: uncaught %s: %s\n",
				typeid(error).name(), error.what());
		}
		catch (...)
		{
			std::fprintf(stderr, "switch: uncaught non-standard exception\n");
		}
	}

	reportMemory("at terminate");
	std::fflush(stderr);
	std::abort();
}

void reportMemory(const char *stage)
{
	u64 total = 0;
	u64 used = 0;

	if (R_FAILED(svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)) ||
		R_FAILED(svcGetInfo(&used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0)))
	{
		return;
	}

	std::fprintf(stderr, "switch: memory %s: used %llu MiB of %llu MiB\n", stage,
		(unsigned long long)(used / (1024 * 1024)),
		(unsigned long long)(total / (1024 * 1024)));
	std::fflush(stderr);
}

void initialize()
{
	const Result romfsResult = romfsInit();
	if (R_FAILED(romfsResult))
		diagAbortWithResult(romfsResult);

	// Pre-create the conventional homebrew directory so the game's
	// File::mkdirs never has to stat the bare "sdmc:" device root.
	::mkdir("sdmc:/switch", 0755);

	// Mesa's KHR_no_error dispatch: skips per-call validation in the GL
	// front end, which the devkitPro OpenGL examples recommend for production
	// (setMesaConfig). The game issues ~1,200 draws per frame, each several
	// GL calls, on a 1 GHz A57. glGetError reports nothing in this mode; the
	// desktop builds keep validation.
	setenv("MESA_NO_ERROR", "1", 1);

	// There is no console on a real unit and emulators do not surface
	// svcOutputDebugString, so stdout and stderr go to the SD card. Unbuffered
	// so a crash cannot lose the tail.
	if (std::freopen("sdmc:/switch/a126cpp.log", "w", stdout) != nullptr)
		std::setvbuf(stdout, nullptr, _IONBF, 0);
	if (std::freopen("sdmc:/switch/a126cpp.log", "a", stderr) != nullptr)
		std::setvbuf(stderr, nullptr, _IONBF, 0);

	std::set_terminate(reportTerminate);
	reportMemory("at startup");

	if (!initializeNetwork())
		std::fprintf(stderr, "switch: no network, online features disabled\n");
}

bool initializeNetwork()
{
	// Networking is optional: without a configured connection the BSD service
	// fails to initialise, which must not stop the game booting.
	if (networkAvailable)
		return true;

	networkAvailable = R_SUCCEEDED(socketInitializeDefault());
	return networkAvailable;
}

void shutdown()
{
	if (networkAvailable)
	{
		socketExit();
		networkAvailable = false;
	}
	romfsExit();
}

bool isNetworkAvailable()
{
	return networkAvailable;
}

}
