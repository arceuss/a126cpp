#pragma once

#include "java/Type.h"

class Timer
{
private:
	static constexpr int_t MAX_TICKS_PER_UPDATE = 10;

public:
	// Alpha Timer.java:19-20,25-27 reads two different clocks and plays them
	// against each other, so both readings have to come from one replaceable
	// source for the timer to be testable without sleeping. The default
	// implementation reads the real clocks and is what the game always uses.
	class Clock
	{
	public:
		virtual ~Clock();

		virtual long_t currentTimeMillis();
		virtual long_t nanoTime();
	};

	static Clock &systemClock();

public:
	float ticksPerSecond = 0.0f;

private:
	double lastTime = 0.0;

public:
	int_t ticks = 0;
	float a = 0.0f;
	float timeScale = 1.0f;
	float passedTime = 0.0f;

private:
	long_t lastMs = 0;
	long_t lastMsSysTime = 0;
	double adjustTime = 1.0;
	Clock *clock = nullptr;

public:
	Timer(float ticksPerSecond);
	Timer(float ticksPerSecond, Clock &clockSource);

	void advanceTime();

	// Alpha keeps timeSyncAdjustment private (Timer.java:15); exposed read-only
	// because the drift correction is otherwise unobservable from a test.
	double getAdjustTime() const { return adjustTime; }
};
