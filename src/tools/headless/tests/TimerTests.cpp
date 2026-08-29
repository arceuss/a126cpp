// Timer parity against Alpha 1.2.6 Timer.java (verified identical in the CFR and
// Vineflower output).
//
// Every test drives a fake clock, so nothing here sleeps or reads the real time.
// The fake starts the monotonic clock at 0 ns, which makes the millisecond
// truncation at Timer.java:20,27,39 exact and every expectation below a closed
// form rather than a recorded number.
//
// Timer.java:41 leaves lastHRTime at 0 until the first updateTimer call, so the
// very first update after construction sees the whole monotonic clock reading as
// its delta. Each test therefore primes the timer with one zero-length update
// before measuring, which is what the game's first frame does too.

#include "client/Timer.h"
#include <string>

#include "tools/headless/TestFramework.h"

// Widened value of the float literal at Timer.java:31. The double 0.2 is a
// different number and would make the drift correction diverge from Java.
static const double TIMER_SYNC_BLEND = static_cast<double>(0.2f);

class TimerTestClock : public Timer::Clock
{
public:
	// A plausible epoch-milliseconds reading; only differences matter.
	long_t wallMs = 1000000000000LL;
	long_t nanos = 0;

	long_t currentTimeMillis() override { return wallMs; }
	long_t nanoTime() override { return nanos; }

	// Both clocks agree, which is the normal case.
	void advance(long_t ms)
	{
		advanceWall(ms);
		advanceMonotonic(ms);
	}

	void advanceWall(long_t ms) { wallMs += ms; }
	void advanceMonotonic(long_t ms) { nanos += ms * 1000000LL; }
};

HEADLESS_TEST(timer, fifty_ms_at_twenty_tps_yields_one_tick)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 0, "priming update must not produce ticks");

	clock.advance(50);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 1, "50 ms at 20 ticks/second");
	ctx.checkEqualBits(timer.passedTime, 0.0f, "no partial tick left over after 50 ms");
	ctx.checkEqualBits(timer.a, timer.passedTime, "renderPartialTicks mirrors elapsedPartialTicks");
}

HEADLESS_TEST(timer, two_successive_fifty_ms_advances_yield_one_tick_each)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	clock.advance(50);
	timer.advanceTime();
	ctx.checkEqual(timer.ticks, 1, "first 50 ms step");

	clock.advance(50);
	timer.advanceTime();
	ctx.checkEqual(timer.ticks, 1, "second 50 ms step");
	ctx.checkEqualBits(timer.passedTime, 0.0f, "second 50 ms step leaves no partial tick");
}

HEADLESS_TEST(timer, sub_tick_advance_accumulates_partial_ticks)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	clock.advance(25);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 0, "half a tick must not round up to a whole tick");
	ctx.checkEqualBits(timer.passedTime, 0.5f, "half a tick is banked as a partial tick");
	ctx.checkEqualBits(timer.a, 0.5f, "renderPartialTicks carries the banked partial tick");

	// The banked half plus another half is a whole tick, which is the point of
	// keeping the remainder at Timer.java:50.
	clock.advance(25);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 1, "two half ticks add up to one tick");
	ctx.checkEqualBits(timer.passedTime, 0.0f, "the banked partial tick was consumed");
}

HEADLESS_TEST(timer, long_stall_clamps_elapsed_ticks_to_ten)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	// 600 ms at 20 ticks/second is 12 ticks. Timer.java:49-53 subtracts the
	// unclamped 12 from elapsedPartialTicks and only then clamps the count, so
	// the two dropped ticks are lost rather than banked; a leftover of 2.0 here
	// would mean the clamp was applied before the subtraction.
	clock.advance(600);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 10, "600 ms of ticks is clamped to 10");
	ctx.checkEqualBits(timer.passedTime, 0.0f, "dropped ticks are discarded, not banked");
}

HEADLESS_TEST(timer, backwards_wall_clock_jump_resyncs_without_negative_ticks)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	clock.advance(50);
	timer.advanceTime();
	ctx.checkEqual(timer.ticks, 1, "normal step before the jump");

	// Wall clock steps back five seconds while the monotonic clock stands still.
	// Elapsed time is measured from the monotonic clock only, so this must
	// produce no work at all - never a negative tick count, which the caller
	// feeds straight into a for loop.
	clock.advanceWall(-5000);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 0, "backwards wall clock jump produces no ticks");
	ctx.checkEqualBits(timer.passedTime, 0.0f, "backwards wall clock jump banks nothing");
	ctx.checkEqualBits(timer.getAdjustTime(), 1.0, "backwards jump must not disturb the drift correction");

	// Timer.java:35-38 resynced both saved clock readings during the jump. Only
	// that resync makes the next 1500 ms wall step exceed the 1000 ms threshold,
	// so reaching the drift branch at all proves the assignment happened; the
	// 3:1 ratio proves lastSyncHRClock was resynced too.
	clock.advanceWall(1500);
	clock.advanceMonotonic(500);
	timer.advanceTime();

	ctx.checkEqualBits(timer.getAdjustTime(), 1.0 + (3.0 - 1.0) * TIMER_SYNC_BLEND,
		"drift correction after the post-jump 1500 ms / 500 ms step");
}

HEADLESS_TEST(timer, monotonic_delta_over_one_second_is_clamped)
{
	TimerTestClock clock;
	// 6.5 ticks/second keeps the result under the 10 tick clamp, so the one
	// second cap is visible in both the tick count and the remainder. At 20
	// ticks/second the tick clamp would hide it.
	Timer timer(6.5f, clock);
	timer.advanceTime();

	// 1500 ms of monotonic time, but only 100 ms of wall time so the drift
	// branch stays out of the way. Timer.java:45-47 caps the work at 1.0 s:
	// 1.0 * 6.5 = 6 ticks with 0.5 banked. Without the cap it would be
	// 1.5 * 6.5 = 9.75, that is 9 ticks with 0.75 banked.
	clock.advanceWall(100);
	clock.advanceMonotonic(1500);
	timer.advanceTime();

	ctx.checkEqual(timer.ticks, 6, "work is capped at one second");
	ctx.checkEqualBits(timer.passedTime, 0.5f, "remainder of the capped one second of work");
}

HEADLESS_TEST(timer, wall_clock_drift_updates_sync_adjustment)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	ctx.checkEqualBits(timer.getAdjustTime(), 1.0, "drift correction starts at 1.0");

	// 2500 ms of wall time against 2000 ms of monotonic time: over the 1000 ms
	// threshold, so Timer.java:28-33 blends the 1.25 ratio in.
	clock.advanceWall(2500);
	clock.advanceMonotonic(2000);
	timer.advanceTime();

	const double firstAdjust = 1.0 + (2500.0 / 2000.0 - 1.0) * TIMER_SYNC_BLEND;
	ctx.checkEqualBits(timer.getAdjustTime(), firstAdjust, "drift correction after 2500 ms / 2000 ms");

	// The blend is a running average, so a second sample compounds on the first.
	clock.advanceWall(1200);
	clock.advanceMonotonic(1000);
	timer.advanceTime();

	ctx.checkEqualBits(timer.getAdjustTime(),
		firstAdjust + (1200.0 / 1000.0 - firstAdjust) * TIMER_SYNC_BLEND,
		"drift correction after a second 1200 ms / 1000 ms sample");

	// A 1000 ms wall step is on the threshold, not over it, so it changes nothing.
	const double heldAdjust = timer.getAdjustTime();
	clock.advance(1000);
	timer.advanceTime();

	ctx.checkEqualBits(timer.getAdjustTime(), heldAdjust,
		"exactly 1000 ms of wall time does not resample the drift correction");
}

HEADLESS_TEST(timer, render_partial_ticks_mirrors_elapsed_partial_ticks)
{
	TimerTestClock clock;
	Timer timer(20.0f, clock);
	timer.advanceTime();

	// Uneven steps, including ones that cross the 1000 ms drift threshold and
	// the 10 tick clamp. The individual remainders are not asserted here because
	// they depend on the whole accumulated history; the invariants are.
	static const long_t steps[] = { 7, 13, 1, 250, 33, 999, 4, 1200, 61 };

	for (int_t i = 0; i < static_cast<int_t>(sizeof(steps) / sizeof(steps[0])); i++)
	{
		clock.advance(steps[i]);
		timer.advanceTime();

		const std::string step = "after a " + std::to_string(steps[i]) + " ms step";
		ctx.checkEqualBits(timer.a, timer.passedTime, "renderPartialTicks mirrors elapsedPartialTicks " + step);
		ctx.check(timer.ticks >= 0 && timer.ticks <= 10, "tick count stays in range " + step);
		ctx.check(timer.passedTime >= 0.0f && timer.passedTime < 1.0f, "partial tick stays in range " + step);
	}
}

HEADLESS_TEST(timer, timer_speed_scales_accumulation)
{
	TimerTestClock doubleClock;
	Timer fast(20.0f, doubleClock);
	fast.advanceTime();
	fast.timeScale = 2.0f;

	doubleClock.advance(25);
	fast.advanceTime();

	ctx.checkEqual(fast.ticks, 1, "half a tick at double speed is a whole tick");
	ctx.checkEqualBits(fast.passedTime, 0.0f, "double speed leaves no partial tick after 25 ms");

	TimerTestClock halfClock;
	Timer slow(20.0f, halfClock);
	slow.advanceTime();
	slow.timeScale = 0.5f;

	halfClock.advance(25);
	slow.advanceTime();

	ctx.checkEqual(slow.ticks, 0, "half a tick at half speed is a quarter tick");
	ctx.checkEqualBits(slow.passedTime, 0.25f, "half speed banks a quarter tick after 25 ms");
}
