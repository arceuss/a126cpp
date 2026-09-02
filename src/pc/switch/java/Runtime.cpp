#include "java/Runtime.h"

#include <malloc.h>

#include <switch.h>

Runtime Runtime::instance;

Runtime &Runtime::getRuntime()
{
	return instance;
}

// libnx claims the whole available heap for the process at startup, so
// svcGetInfo(UsedMemorySize) reports ~everything before the game allocates
// anything and is useless for tracking the game's own footprint. newlib's
// malloc arena is the real signal, and it maps directly onto Java's contract:
//
//   arena    total heap space obtained  -> totalMemory
//   fordblks free space inside it       -> freeMemory
//   arena - fordblks                    -> what the debug overlay shows as used
//
// maxMemory stays the process ceiling, which is what the overlay divides by.
long_t Runtime::maxMemory()
{
	u64 total = 0;
	if (R_FAILED(svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)) ||
		total == 0)
	{
		return 1;
	}
	return static_cast<long_t>(total);
}

// newlib's mallinfo walks every chunk in the arena. With the game's heap that
// measured 3.4 ms per call on the console, and the debug overlay asks twice a
// frame, so the answer is sampled at most once a second. Java's counters are
// O(1); an overlay that lags by a second is the same information.
static const struct mallinfo &sampledMallinfo()
{
	static struct mallinfo sample = {};
	static u64 sampledAt = 0;
	const u64 now = armTicksToNs(armGetSystemTick());
	if (sampledAt == 0 || now - sampledAt >= 1000000000ull)
	{
		sample = mallinfo();
		sampledAt = now;
	}
	return sample;
}

long_t Runtime::totalMemory()
{
	return static_cast<long_t>(sampledMallinfo().arena);
}

long_t Runtime::freeMemory()
{
	return static_cast<long_t>(sampledMallinfo().fordblks);
}
