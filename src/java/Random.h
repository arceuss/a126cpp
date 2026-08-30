#pragma once

#include "java/Type.h"

// Reproduction of java.util.Random.
//
// Every value and every bit consumed must match Java exactly: world
// generation, mob spawning and drop tables all depend on the number of calls
// made as much as on the numbers produced.
class Random
{
private:
	// Held unsigned so the linear congruential step wraps instead of
	// overflowing signed arithmetic.
	ulong_t seed;

	double nextNextGaussian = 0.0;
	bool haveNextNextGaussian = false;

public:
	Random();
	Random(long_t set_seed);
	// Scene capture is one process with no later gameplay. This makes every
	// subsequent no-argument Random reproducible without changing the default.
	static void setDefaultSeedForCapture(long_t seed);

	void setSeed(long_t set_seed);

	int_t next(int_t bits);

	bool nextBoolean();

	int_t nextInt();
	int_t nextInt(int_t bound);

	long_t nextLong();

	float nextFloat();
	double nextDouble();

	double nextGaussian();
};
