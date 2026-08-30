#pragma once

#include <string>

// Developer fixture: renders a wall of signs in a hidden window and logs frame
// times. `blankText` leaves every line empty, which separates the cost of the
// sign boards from the cost of the text. `finishEachFrame` attributes all queued
// GPU work to its producing frame; disabling it measures normal multi-frame
// cadence. A non-empty `worldName` loads that saved world instead of generating
// the sign wall.
int runSignBench(int frames, int signCount, bool blankText, bool finishEachFrame,
	const std::string &worldName);

// Developer fixture: measures how many game ticks the real timer produces per
// wall-clock second, using the same call sequence as `Minecraft::run`.
int runTimerProbe(int seconds);
