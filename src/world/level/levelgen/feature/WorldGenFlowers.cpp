#include "world/level/levelgen/feature/WorldGenFlowers.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/MushroomTile.h"
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
		// Alpha: air, and the plant itself decides whether it may stay, so
		// mushrooms accept any opaque block while flowers need soil and light
		// (WorldGenFlowers.java:25, Block.java:521-523)
		if (level.getTile(px, py, pz) == 0 && Tile::tiles[plantBlockId]->canBlockStay(level, px, py, pz))
			level.setTileNoUpdate(px, py, pz, plantBlockId);  // Alpha: setBlock, no neighbour notify (WorldGenFlowers.java:26)
	}
	return true;
}
