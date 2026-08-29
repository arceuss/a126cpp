// Nether terrain regressions.
//
// Alpha reference:
//   WorldProviderHell.java:39-41   binds ChunkProviderHell to dimension -1
//   ChunkProviderHell.java:56-110  lava below y=32, netherrack where the noise
//                                  is positive, air everywhere else
//   ChunkProviderHell.java:112-179 bedrock roof and floor, gravel and soul sand
//                                  patches, no overworld surface blocks

#include <map>
#include <memory>
#include <set>
#include <string>

#include "world/level/LightLayer.h"
#include "world/level/dimension/Dimension.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FireTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/GravelTile.h"
#include "world/level/tile/HellSandTile.h"
#include "world/level/tile/HellStoneTile.h"
#include "world/level/tile/LightGemTile.h"
#include "world/level/tile/MushroomTile.h"
#include "world/level/tile/UnbreakableTile.h"

static int_t netherTileAt(LevelChunk &chunk, int_t x, int_t y, int_t z)
{
	return chunk.blocks[static_cast<size_t>((x * 16 + z) * 128 + y)];
}

HEADLESS_TEST(nethergen, hell_dimension_generates_netherrack_world)
{
	headless::initGameRegistries();
	Level level(u"nethergen-terrain", Dimension::Id_Hell, 0x126LL);

	// Every tile Alpha's hell generator and its decorators can place.
	const std::set<int_t> allowed = {
		0,
		Tile::unbreakable.id,
		Tile::lava.id,
		Tile::calmLava.id,
		Tile::gravel.id,
		Tile::fire.id,
		Tile::mushroomBrown.id,
		Tile::mushroomRed.id,
		Tile::hellRock.id,
		Tile::hellSand.id,
		Tile::lightGem.id
	};

	std::set<int_t> unexpected;
	long long netherrack = 0;
	long long lavaBelowSea = 0;
	bool roofIsBedrock = true;
	bool floorIsBedrock = true;

	for (int_t cx = 0; cx < 4; cx++)
	{
		for (int_t cz = 0; cz < 4; cz++)
		{
			std::shared_ptr<LevelChunk> chunk = level.getChunk(cx, cz);
			if (!ctx.check(chunk != nullptr, "the nether chunk generates"))
				return;

			for (int_t x = 0; x < 16; x++)
			{
				for (int_t z = 0; z < 16; z++)
				{
					if (netherTileAt(*chunk, x, 127, z) != Tile::unbreakable.id)
						roofIsBedrock = false;
					if (netherTileAt(*chunk, x, 0, z) != Tile::unbreakable.id)
						floorIsBedrock = false;

					for (int_t y = 0; y < 128; y++)
					{
						int_t tile = netherTileAt(*chunk, x, y, z);
						if (allowed.find(tile) == allowed.end())
							unexpected.insert(tile);
						if (tile == Tile::hellRock.id)
							netherrack++;
						if (y < 32 && (tile == Tile::lava.id || tile == Tile::calmLava.id))
							lavaBelowSea++;
					}
				}
			}
		}
	}

	for (int_t tile : unexpected)
		ctx.fail("overworld tile " + std::to_string(tile) + " was generated in the nether");

	ctx.check(roofIsBedrock, "y=127 is a solid bedrock ceiling");
	ctx.check(floorIsBedrock, "y=0 is a solid bedrock floor");
	ctx.check(netherrack > 0, "the nether is built out of netherrack");
	ctx.check(lavaBelowSea > 0, "a lava sea fills the bottom of the nether");
}

HEADLESS_TEST(nethergen, hell_generation_repeats_for_one_seed)
{
	headless::initGameRegistries();
	Level first(u"nethergen-repeat-a", Dimension::Id_Hell, 0x5EEDLL);
	Level second(u"nethergen-repeat-b", Dimension::Id_Hell, 0x5EEDLL);

	std::shared_ptr<LevelChunk> a = first.getChunk(2, -3);
	std::shared_ptr<LevelChunk> b = second.getChunk(2, -3);
	if (!ctx.check(a != nullptr && b != nullptr, "both nether chunks generate"))
		return;

	ctx.check(a->blocks == b->blocks, "one seed always produces the same nether chunk");
}

// Alpha's ChunkProviderHell.populate runs six decoration passes
// (ChunkProviderHell.java:281-329). Each one must actually place blocks: the
// mushroom passes were silently dead while WorldGenFlowers hardcoded a
// grass-or-dirt test instead of asking the plant, and Alpha's mushrooms accept
// any opaque block below in light 13 or less (BlockMushroom.java:18-24).
HEADLESS_TEST(nethergen, decoration_passes_place_their_blocks)
{
	headless::initGameRegistries();
	Level level(u"nethergen-decoration", Dimension::Id_Hell, 0x126LL);

	std::map<int_t, long long> counts;
	for (int_t cx = 0; cx < 12; cx++)
	{
		for (int_t cz = 0; cz < 12; cz++)
		{
			std::shared_ptr<LevelChunk> chunk = level.getChunk(cx, cz);
			for (size_t i = 0; i < chunk->blocks.size(); i++)
				counts[chunk->blocks[i]]++;
		}
	}

	ctx.check(counts[Tile::gravel.id] > 0, "surface gravel patches are generated");
	ctx.check(counts[Tile::hellSand.id] > 0, "surface soul sand patches are generated");
	ctx.check(counts[Tile::lava.id] > 0, "hell lava springs are generated");
	ctx.check(counts[Tile::fire.id] > 0, "fire is generated");
	ctx.check(counts[Tile::lightGem.id] > 0, "glowstone clusters are generated");
	ctx.check(counts[Tile::mushroomBrown.id] > 0, "brown mushrooms are generated");
	ctx.check(counts[Tile::mushroomRed.id] > 0, "red mushrooms are generated");
}


// Alpha: WorldProviderHell raises the ambient floor of the brightness table to
// 0.1 and sets the no-sky flag, so the nether never receives sky light
// (WorldProviderHell.java:23,31-37; Chunk.java:246; World.java:601,1341).
HEADLESS_TEST(nethergen, hell_lighting_matches_alpha)
{
	headless::initGameRegistries();
	Level overworld(u"nethergen-light-overworld", Dimension::Id_Normal, 0x126LL);
	Level nether(u"nethergen-light-hell", Dimension::Id_Hell, 0x126LL);

	ctx.checkEqualBits(overworld.dimension->brightnessRamp[0], 0.05f,
		"the overworld ambient floor stays at 0.05");
	ctx.checkEqualBits(nether.dimension->brightnessRamp[0], 0.1f,
		"the nether ambient floor is 0.1");
	ctx.checkEqualBits(nether.dimension->brightnessRamp[15], 1.0f,
		"full block light still reaches 1.0");

	ctx.check(nether.dimension->hasCeiling, "the nether has a ceiling, so sky light is disabled");
	ctx.check(nether.dimension->foggy, "the nether is foggy, which also drops the sky and clouds");
	ctx.check(nether.dimension->ultraWarm, "the nether is ultra warm, so water evaporates and lava runs far");
	ctx.checkEqual(nether.dimension->id, -1, "the nether dimension id is -1");
	ctx.checkEqualBits(nether.dimension->getTimeOfDay(6000LL, 1.0f), 0.5f,
		"the nether has no day cycle");

	// A generated nether column must carry no sky light at any height.
	nether.getChunk(0, 0);
	int_t skyLight = 0;
	for (int_t y = 0; y < 128; y++)
		skyLight += nether.getBrightness(LightLayer::Sky, 8, y, 8);
	ctx.checkEqual(skyLight, 0, "no sky light is stored anywhere in a nether column");
}