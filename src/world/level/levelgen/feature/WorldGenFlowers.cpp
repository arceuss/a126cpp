#include "world/level/levelgen/feature/WorldGenFlowers.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "java/Random.h"

// Alpha: WorldGenFlowers.java
WorldGenFlowers::WorldGenFlowers(int_t blockId) : plantBlockId(blockId)
{
}

bool WorldGenFlowers::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenFlowers.java:12-20
	// 64 attempts to place the plant
	for (int_t i = 0; i < 64; ++i)
	{
		int_t px = x + random.nextInt(8) - random.nextInt(8);
		int_t py = y + random.nextInt(4) - random.nextInt(4);
		int_t pz = z + random.nextInt(8) - random.nextInt(8);
		
		// Alpha: Check if block is air and canBlockStay (delegates to BlockFlower.canBlockStay)
		if (level.getTile(px, py, pz) == 0)
		{
			// TODO: Check BlockFlower.canBlockStay() - for now, check if block below is grass/dirt
			int_t belowId = level.getTile(px, py - 1, pz);
			if (belowId == Tile::grass.id || belowId == Tile::dirt.id)
			{
				level.setTile(px, py, pz, plantBlockId);
			}
		}
	}
	return true;
}
