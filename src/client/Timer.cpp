#include "Timer.h"

#include "java/System.h"

Timer::Clock::~Clock() = default;

long_t Timer::Clock::currentTimeMillis()
{
	return System::currentTimeMillis();
}

long_t Timer::Clock::nanoTime()
{
	return System::nanoTime();
}

Timer::Clock &Timer::systemClock()
{
	static Clock instance;
	return instance;
}

Timer::Timer(float ticksPerSecond)
	: Timer(ticksPerSecond, systemClock())
{
}

// Alpha Timer.java:17-21
Timer::Timer(float ticksPerSecond, Clock &clockSource)
{
	clock = &clockSource;
	this->ticksPerSecond = ticksPerSecond;
	lastMs = clock->currentTimeMillis();
	lastMsSysTime = clock->nanoTime() / 1000000L;
}

// Alpha Timer.java:23-55 (updateTimer). The wall clock is never used to measure
// elapsed time; it only detects that the monotonic clock runs at the wrong rate
// (passedMs > 1000) and that the wall clock jumped backwards (passedMs < 0).
void Timer::advanceTime()
{
	long_t nowMs = clock->currentTimeMillis();
	long_t passedMs = nowMs - lastMs;
	long_t msSysTime = clock->nanoTime() / 1000000L;

	if (passedMs > 1000)
	{
		long_t passedMsSysTime = msSysTime - lastMsSysTime;

		// Alpha Timer.java:30 divides as double, so a zero monotonic delta
		// yields infinity here rather than trapping.
		double adjustTimeT = static_cast<double>(passedMs) / static_cast<double>(passedMsSysTime);
		// Alpha Timer.java:31 mixes in a float literal, so the running average
		// uses the widened value of 0.2f, not the double 0.2.
		adjustTime += (adjustTimeT - adjustTime) * static_cast<double>(0.2f);

		lastMs = nowMs;
		lastMsSysTime = msSysTime;
	}

	if (passedMs < 0)
	{
		lastMs = nowMs;
		lastMsSysTime = msSysTime;
	}

	// Alpha Timer.java:20,27,39 truncates nanoTime to whole milliseconds before
	// converting to seconds. The lost sub-millisecond precision is behaviour.
	double now = static_cast<double>(msSysTime) / 1000.0;
	double passedSeconds = (now - lastTime) * adjustTime;
	lastTime = now;

	if (passedSeconds < 0.0) passedSeconds = 0.0;
	if (passedSeconds > 1.0) passedSeconds = 1.0;

	// Alpha Timer.java:48 accumulates in double and stores back into a float.
	passedTime = static_cast<float>(static_cast<double>(passedTime) + passedSeconds * static_cast<double>(timeScale) * static_cast<double>(ticksPerSecond));

	// Alpha Timer.java:49-53 subtracts the unclamped tick count before clamping,
	// so a long stall drops the excess ticks instead of banking them.
	ticks = static_cast<int_t>(passedTime);
	passedTime -= static_cast<float>(ticks);
	if (ticks > MAX_TICKS_PER_UPDATE) ticks = MAX_TICKS_PER_UPDATE;
	a = passedTime;
}
