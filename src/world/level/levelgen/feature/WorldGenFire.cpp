#include "world/level/levelgen/feature/WorldGenFire.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/HellStoneTile.h"
#include "world/level/tile/FireTile.h"
#include "java/Random.h"

WorldGenFire::WorldGenFire()
{
}

bool WorldGenFire::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenFire.java:14-20 - 64 attempts to seat fire on netherrack
	for (int_t i = 0; i < 64; ++i)
	{
		int_t px = x + random.nextInt(8) - random.nextInt(8);
		int_t py = y + random.nextInt(4) - random.nextInt(4);
		int_t pz = z + random.nextInt(8) - random.nextInt(8);

		// Alpha: WorldGenFire.java:18 - air with netherrack directly below
		if (level.getTile(px, py, pz) != 0 || level.getTile(px, py - 1, pz) != Tile::hellRock.id)
		{
			continue;
		}

		level.setTile(px, py, pz, Tile::fire.id);
	}

	// Alpha: WorldGenFire.java:21
	return true;
}
