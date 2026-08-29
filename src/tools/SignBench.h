#pragma once

// Developer fixture: renders a wall of signs in a window and logs frame times.
// `blankText` leaves every line empty, which separates the cost of the sign
// boards from the cost of the text.
int runSignBench(int frames, int signCount, bool blankText);

// Developer fixture: measures how many game ticks the real timer produces per
// wall-clock second, using the same call sequence as `Minecraft::run`.
int runTimerProbe(int seconds);
