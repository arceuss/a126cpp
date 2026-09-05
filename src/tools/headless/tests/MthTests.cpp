#include <cstdint>
#include <cstring>
#include "tools/headless/TestFramework.h"
#include "tools/headless/oracle/MthOracleData.h"
#include "util/Mth.h"

static uint_t floatBits(float value)
{
	uint_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static ulong_t hashSample(ulong_t hash, float value)
{
	const uint_t bits = floatBits(value);
	for (int i = 0; i < 4; ++i)
		hash = (hash ^ ((bits >> (i * 8)) & 255U)) * 0x100000001b3ULL;
	return hash;
}

HEADLESS_TEST(mth, every_sine_index_matches_alpha_java)
{
	ulong_t sinHash = 0xcbf29ce484222325ULL;
	ulong_t cosHash = sinHash;
	for (int i = 0; i < 65536; ++i)
	{
		const float angle = (i + 0.25f) / 10430.378f;
		sinHash = hashSample(sinHash, Mth::sin(angle));
		cosHash = hashSample(cosHash, Mth::cos(angle));
	}
	ctx.checkEqual(sinHash, headless::mthSinHash, "all 65536 sine indices, Java raw float bits");
	ctx.checkEqual(cosHash, headless::mthCosHash, "cosine offset expression, Java raw float bits");
}

HEADLESS_TEST(mth, trig_index_conversion_matches_alpha_java)
{
	for (const headless::MthCase &sample : headless::mthCases)
	{
		float angle;
		std::memcpy(&angle, &sample.angle, sizeof(angle));
		ctx.checkEqual(floatBits(Mth::sin(angle)), sample.sin, "sine conversion including NaN and saturation");
		ctx.checkEqual(floatBits(Mth::cos(angle)), sample.cos, "cosine conversion including NaN and saturation");
	}
}
