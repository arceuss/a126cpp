#include "java/Random.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "java/StrictMath.h"

static constexpr ulong_t RANDOM_MUL = 0x5DEECE66DULL;
static constexpr ulong_t RANDOM_ADD = 0xBULL;
static constexpr ulong_t RANDOM_AND = (1ULL << 48) - 1;

static bool useCaptureDefaultSeed = false;
static long_t captureDefaultSeed = 0;

// Reinterpret the low 32 bits as a Java int. A plain conversion of a value
// above INT_MAX is implementation defined before C++20, so go through the
// object representation.
static int_t toJavaInt(uint32_t bits)
{
	int_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

static long_t toJavaLong(ulong_t bits)
{
	long_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

Random::Random()
{
	if (useCaptureDefaultSeed)
	{
		setSeed(captureDefaultSeed);
		return;
	}

	// Java seeds the no-argument constructor from a uniquifier combined with
	// System.nanoTime(), which is not reproducible by design. Only the seeded
	// constructor carries a parity guarantee.
	auto now = std::chrono::steady_clock::now();
	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	setSeed(static_cast<long_t>(nanos));
}

Random::Random(long_t set_seed)
{
	setSeed(set_seed);
}

void Random::setDefaultSeedForCapture(long_t seed)
{
	captureDefaultSeed = seed;
	useCaptureDefaultSeed = true;
}

void Random::setSeed(long_t set_seed)
{
	seed = (static_cast<ulong_t>(set_seed) ^ RANDOM_MUL) & RANDOM_AND;
	// Java's setSeed discards the cached second Gaussian value.
	haveNextNextGaussian = false;
	nextNextGaussian = 0.0;
}

int_t Random::next(int_t bits)
{
	seed = (seed * RANDOM_MUL + RANDOM_ADD) & RANDOM_AND;
	return toJavaInt(static_cast<uint32_t>(seed >> (48 - bits)));
}

bool Random::nextBoolean()
{
	return next(1) != 0;
}

int_t Random::nextInt()
{
	return next(32);
}

int_t Random::nextInt(int_t bound)
{
	// Verify that our bound is positive and non-zero
	if (bound <= 0)
		throw std::invalid_argument("bound must be positive");

	int_t r = next(31);
	const int_t m = bound - 1;
	if ((bound & m) == 0) // ie Bound is a power of 2
	{
		r = static_cast<int_t>((static_cast<long_t>(bound) * static_cast<long_t>(r)) >> 31);
	}
	else
	{
		// Reject over-represented candidates. Java evaluates u - r + m in
		// wrapping 32-bit arithmetic and tests the sign of the result, so the
		// intermediate has to be computed unsigned to stay defined here.
		for (int_t u = r;; u = next(31))
		{
			r = u % bound;
			const uint32_t wrapped = static_cast<uint32_t>(u) - static_cast<uint32_t>(r) + static_cast<uint32_t>(m);
			if (toJavaInt(wrapped) >= 0)
				break;
		}
	}
	return r;
}

long_t Random::nextLong()
{
	// Java: ((long)next(32) << 32) + next(32), with both calls made in this
	// order and the sum wrapping.
	const int_t high = next(32);
	const int_t low = next(32);
	const ulong_t combined = (static_cast<ulong_t>(static_cast<uint32_t>(high)) << 32)
		+ static_cast<ulong_t>(static_cast<long_t>(low));
	return toJavaLong(combined);
}

float Random::nextFloat()
{
	return next(24) / static_cast<float>(1 << 24);
}

double Random::nextDouble()
{
	// Java: (((long)next(26) << 27) + next(27)) * 0x1.0p-53
	const int_t high = next(26);
	const int_t low = next(27);
	const long_t mantissa = (static_cast<long_t>(high) << 27) + low;
	return static_cast<double>(mantissa) * 0x1.0p-53;
}

double Random::nextGaussian()
{
	// Java's polar method: two uniforms are consumed per pair of results and
	// the second result is cached, so the number of nextDouble calls depends
	// on the call pattern. Reproducing that matters as much as the values.
	if (haveNextNextGaussian)
	{
		haveNextNextGaussian = false;
		return nextNextGaussian;
	}

	double v1, v2, s;
	do
	{
		v1 = 2 * nextDouble() - 1;
		v2 = 2 * nextDouble() - 1;
		s = v1 * v1 + v2 * v2;
	}
	while (s >= 1 || s == 0);

	// StrictMath, not the host libm: std::log can differ by one ULP and that
	// difference propagates into gameplay values.
	const double multiplier = std::sqrt(-2 * StrictMath::log(s) / s);
	nextNextGaussian = v2 * multiplier;
	haveNextNextGaussian = true;
	return v1 * multiplier;
}
