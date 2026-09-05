#include <array>
#include <string>
#include "tools/headless/TestFramework.h"
#include "tools/headless/oracle/CaveOracleData.h"
#include "world/level/levelgen/LargeCaveFeature.h"

class FixtureCave : public LargeCaveFeature
{
public:
	void carve(std::array<ubyte_t, 32768> &blocks, int fixture)
	{
		random.setSeed(123456789LL);
		addTunnel(0, 0, blocks, 8.0, fixture == 2 ? 40.0 : 8.0, 8.0,
			3.0f, 0.0f, 0.0f, fixture == 2 ? 0 : -1, 24, 0.5);
	}
};

static void compareCave(headless::TestContext &ctx, int fixture, const headless::CaveRun *runs)
{
	std::array<ubyte_t, 32768> blocks;
	blocks.fill(1);
	if (fixture == 1)
		for (int x = 0; x < 16; ++x)
			for (int z = 0; z < 16; ++z)
				blocks[(x * 16 + z) * 128 + 12] = 9;
	FixtureCave cave;
	cave.carve(blocks, fixture);
	std::size_t index = 0;
	while (index < blocks.size())
	{
		for (int i = 0; i < runs->count; ++i, ++index)
		{
			if (!ctx.checkEqual(blocks[index], runs->tile, "first differing cave voxel x=" +
				std::to_string(index / 2048) + " y=" + std::to_string(index % 128) +
				" z=" + std::to_string((index / 128) % 16)))
				return;
		}
		++runs;
	}
}

HEADLESS_TEST(worldgen, cave_lava_matches_alpha_bytecode)
{
	compareCave(ctx, 0, headless::cave0);
}
HEADLESS_TEST(worldgen, cave_water_rejection_matches_alpha_bytecode)
{
	compareCave(ctx, 1, headless::cave1);
}
HEADLESS_TEST(worldgen, branching_tunnel_matches_alpha_bytecode)
{
	compareCave(ctx, 2, headless::cave2);
}
