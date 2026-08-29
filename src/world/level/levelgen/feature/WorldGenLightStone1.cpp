#include "world/level/levelgen/feature/WorldGenLightStone1.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/HellStoneTile.h"
#include "world/level/tile/LightGemTile.h"
#include "java/Random.h"

WorldGenLightStone1::WorldGenLightStone1()
{
}

bool WorldGenLightStone1::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenLightStone1.java:14-16 - anchor must be air
	if (level.getTile(x, y, z) != 0)
	{
		return false;
	}

	// Alpha: WorldGenLightStone1.java:17-19 - and hang from netherrack
	if (level.getTile(x, y + 1, z) != Tile::hellRock.id)
	{
		return false;
	}

	// Alpha: WorldGenLightStone1.java:20 - seed block of the cluster
	level.setTile(x, y, z, Tile::lightGem.id);

	// Alpha: WorldGenLightStone1.java:21-52 - grow the cluster downwards
	for (int_t i = 0; i < 1500; ++i)
	{
		int_t px = x + random.nextInt(8) - random.nextInt(8);
		int_t py = y - random.nextInt(12);
		int_t pz = z + random.nextInt(8) - random.nextInt(8);

		if (level.getTile(px, py, pz) != 0)
		{
			continue;
		}

		// Alpha: WorldGenLightStone1.java:26-49 - count adjacent glowstone
		int_t neighbors = 0;
		for (int_t face = 0; face < 6; ++face)
		{
			int_t tileId = 0;
			if (face == 0)
			{
				tileId = level.getTile(px - 1, py, pz);
			}
			if (face == 1)
			{
				tileId = level.getTile(px + 1, py, pz);
			}
			if (face == 2)
			{
				tileId = level.getTile(px, py - 1, pz);
			}
			if (face == 3)
			{
				tileId = level.getTile(px, py + 1, pz);
			}
			if (face == 4)
			{
				tileId = level.getTile(px, py, pz - 1);
			}
			if (face == 5)
			{
				tileId = level.getTile(px, py, pz + 1);
			}
			if (tileId != Tile::lightGem.id)
			{
				continue;
			}
			++neighbors;
		}

		// Alpha: WorldGenLightStone1.java:50 - attach only to a single existing block
		if (neighbors != 1)
		{
			continue;
		}

		level.setTile(px, py, pz, Tile::lightGem.id);
	}

	// Alpha: WorldGenLightStone1.java:53
	return true;
}
