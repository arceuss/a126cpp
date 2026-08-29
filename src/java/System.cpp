#include "System.h"

#include <chrono>

namespace System
{

long_t currentTimeMillis()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

long_t nanoTime()
{
	// Java's System.nanoTime is monotonic. std::chrono::high_resolution_clock
	// carries no such guarantee and may alias system_clock, which would let the
	// wall clock drag the monotonic reading backwards; steady_clock is the only
	// standard clock that promises monotonicity.
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}