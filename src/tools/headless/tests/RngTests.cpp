// Layer 1: java.util.Random parity.
//
// The expected values come from tools/oracle/RandomOracle.java run on a JDK 8
// runtime, so these tests compare against real Java output rather than a
// restatement of the C++ implementation.

#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

#include "java/Random.h"
#include "java/StrictMath.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/oracle/RandomOracleData.h"
#include "tools/headless/oracle/StrictMathOracleData.h"

static std::string stepLabel(long long seed, int index, const char *op, int arg)
{
	std::ostringstream text;
	text << "seed " << seed << " step " << index << ' ' << op;
	if (arg != 0)
		text << '(' << arg << ')';
	return text.str();
}

static const char *opName(headless::RandomOp op)
{
	switch (op)
	{
		case headless::RandomOp::Next: return "next";
		case headless::RandomOp::NextInt: return "nextInt";
		case headless::RandomOp::NextIntBound: return "nextInt";
		case headless::RandomOp::NextLong: return "nextLong";
		case headless::RandomOp::NextFloatBits: return "nextFloat";
		case headless::RandomOp::NextDoubleBits: return "nextDouble";
		case headless::RandomOp::NextGaussianBits: return "nextGaussian";
		case headless::RandomOp::NextBoolean: return "nextBoolean";
	}
	return "?";
}

static unsigned long long rawBits(float value)
{
	uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static unsigned long long rawBits(double value)
{
	uint64_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

// Replays the recorded operation script and reports the first divergence per
// group; a single wrong call desynchronises everything after it, so listing
// every later step would be noise.
static void replay(headless::TestContext &ctx, const headless::RandomOracleGroup &group,
	bool includeGaussian, bool onlyGaussian)
{
	Random random(group.seed);

	for (int i = 0; i < group.stepCount; ++i)
	{
		const headless::RandomOracleStep &step = group.steps[i];
		const bool isGaussian = step.op == headless::RandomOp::NextGaussianBits;
		if (isGaussian && !includeGaussian)
			return; // Cannot continue: the stream position would drift.

		unsigned long long actual = 0;
		switch (step.op)
		{
			case headless::RandomOp::Next:
				actual = static_cast<uint32_t>(random.next(step.arg));
				break;
			case headless::RandomOp::NextInt:
				actual = static_cast<uint32_t>(random.nextInt());
				break;
			case headless::RandomOp::NextIntBound:
				actual = static_cast<uint32_t>(random.nextInt(step.arg));
				break;
			case headless::RandomOp::NextLong:
				actual = static_cast<unsigned long long>(random.nextLong());
				break;
			case headless::RandomOp::NextFloatBits:
				actual = rawBits(random.nextFloat());
				break;
			case headless::RandomOp::NextDoubleBits:
				actual = rawBits(random.nextDouble());
				break;
			case headless::RandomOp::NextGaussianBits:
				actual = rawBits(random.nextGaussian());
				break;
			case headless::RandomOp::NextBoolean:
				actual = random.nextBoolean() ? 1ULL : 0ULL;
				break;
		}

		if (onlyGaussian && !isGaussian)
			continue;

		if (actual != step.expected)
		{
			std::ostringstream text;
			text << stepLabel(group.seed, i, opName(step.op), step.arg)
				<< ": expected 0x" << std::hex << step.expected
				<< ", got 0x" << actual;
			ctx.fail(text.str());
			return;
		}
	}
}

HEADLESS_TEST(rng, next_bits_matches_java)
{
	for (int g = 0; g < headless::randomOracleGroupCount; ++g)
	{
		const headless::RandomOracleGroup &group = headless::randomOracleGroups[g];
		Random random(group.seed);
		for (int i = 0; i < group.stepCount; ++i)
		{
			const headless::RandomOracleStep &step = group.steps[i];
			if (step.op != headless::RandomOp::Next)
				break;
			const unsigned long long actual = static_cast<uint32_t>(random.next(step.arg));
			if (actual != step.expected)
			{
				std::ostringstream text;
				text << stepLabel(group.seed, i, "next", step.arg)
					<< ": expected 0x" << std::hex << step.expected << ", got 0x" << actual;
				ctx.fail(text.str());
				break;
			}
		}
	}
}

HEADLESS_TEST(rng, integer_and_float_stream_matches_java)
{
	// Excludes nextGaussian so a StrictMath problem cannot mask an error in
	// the far more widely used integer and float paths.
	for (int g = 0; g < headless::randomOracleGroupCount; ++g)
		replay(ctx, headless::randomOracleGroups[g], false, false);
}

HEADLESS_TEST(rng, gaussian_bits_match_java)
{
	for (int g = 0; g < headless::randomOracleGroupCount; ++g)
		replay(ctx, headless::randomOracleGroups[g], true, true);
}

HEADLESS_TEST(rng, full_stream_matches_java)
{
	for (int g = 0; g < headless::randomOracleGroupCount; ++g)
		replay(ctx, headless::randomOracleGroups[g], true, false);
}

HEADLESS_TEST(rng, next_double_uses_26_and_27_bits)
{
	// Pins the exact construction rather than only the resulting values: a
	// 27+27 bit variant happens to produce plausible-looking doubles.
	Random reference(1234LL);
	const int_t high = reference.next(26);
	const int_t low = reference.next(27);
	const double expected = static_cast<double>((static_cast<long_t>(high) << 27) + low) * 0x1.0p-53;

	Random actual(1234LL);
	ctx.checkEqualBits(actual.nextDouble(), expected, "nextDouble construction");
}

HEADLESS_TEST(rng, set_seed_resets_gaussian_cache)
{
	Random random(7L);
	random.nextGaussian(); // Leaves the paired value cached.
	random.setSeed(7L);

	Random fresh(7L);
	ctx.checkEqualBits(random.nextGaussian(), fresh.nextGaussian(),
		"setSeed must discard the cached Gaussian");
}

HEADLESS_TEST(rng, bounded_next_int_rejects_non_positive_bounds)
{
	Random random(3L);
	bool threw = false;
	try
	{
		random.nextInt(0);
	}
	catch (const std::exception &)
	{
		threw = true;
	}
	ctx.check(threw, "nextInt(0) must throw like Java");

	threw = false;
	try
	{
		random.nextInt(-5);
	}
	catch (const std::exception &)
	{
		threw = true;
	}
	ctx.check(threw, "nextInt(-5) must throw like Java");
}

HEADLESS_TEST(rng, strict_math_log_is_bit_exact)
{
	// Oracle covers every fdlibm branch plus the argument range nextGaussian
	// actually feeds to log.
	for (int i = 0; i < headless::strictMathLogCaseCount; ++i)
	{
		const headless::StrictMathLogCase &entry = headless::strictMathLogCases[i];

		double input = 0.0;
		std::memcpy(&input, &entry.inputBits, sizeof(input));

		const double actual = StrictMath::log(input);
		unsigned long long actualBits = 0;
		std::memcpy(&actualBits, &actual, sizeof(actualBits));

		if (actualBits != entry.expectedBits)
		{
			std::ostringstream text;
			text << "StrictMath::log(" << entry.label << "): expected 0x" << std::hex
				<< entry.expectedBits << ", got 0x" << actualBits;
			ctx.fail(text.str());
		}
	}
}
