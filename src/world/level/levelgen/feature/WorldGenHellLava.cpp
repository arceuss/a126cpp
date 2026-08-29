#include "world/level/levelgen/feature/WorldGenHellLava.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/HellStoneTile.h"
#include "java/Random.h"

// Alpha: WorldGenHellLava.java:15-17
WorldGenHellLava::WorldGenHellLava(int_t tileId) : liquidTileId(tileId)
{
}

bool WorldGenHellLava::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenHellLava.java:20-22 - must hang below netherrack
	if (level.getTile(x, y + 1, z) != Tile::hellRock.id)
	{
		return false;
	}

	// Alpha: WorldGenHellLava.java:23-25 - centre must be air or netherrack
	if (level.getTile(x, y, z) != 0 && level.getTile(x, y, z) != Tile::hellRock.id)
	{
		return false;
	}

	// Alpha: WorldGenHellLava.java:26-41 - count netherrack neighbours
	int_t hellRockCount = 0;
	if (level.getTile(x - 1, y, z) == Tile::hellRock.id)
	{
		++hellRockCount;
	}
	if (level.getTile(x + 1, y, z) == Tile::hellRock.id)
	{
		++hellRockCount;
	}
	if (level.getTile(x, y, z - 1) == Tile::hellRock.id)
	{
		++hellRockCount;
	}
	if (level.getTile(x, y, z + 1) == Tile::hellRock.id)
	{
		++hellRockCount;
	}
	if (level.getTile(x, y - 1, z) == Tile::hellRock.id)
	{
		++hellRockCount;
	}

	// Alpha: WorldGenHellLava.java:42-57 - count air neighbours
	int_t airCount = 0;
	if (level.getTile(x - 1, y, z) == 0)
	{
		++airCount;
	}
	if (level.getTile(x + 1, y, z) == 0)
	{
		++airCount;
	}
	if (level.getTile(x, y, z - 1) == 0)
	{
		++airCount;
	}
	if (level.getTile(x, y, z + 1) == 0)
	{
		++airCount;
	}
	if (level.getTile(x, y - 1, z) == 0)
	{
		++airCount;
	}

	// Alpha: WorldGenHellLava.java:58-63 - four netherrack sides, one open side
	if (hellRockCount == 4 && airCount == 1)
	{
		level.setTile(x, y, z, liquidTileId);
		// Alpha: world.field_4214_a (WorldGenHellLava.java:60) - tick the fluid immediately
		level.instaTick = true;
		Tile::tiles[liquidTileId]->tick(level, x, y, z, random);
		level.instaTick = false;
	}

	// Alpha: WorldGenHellLava.java:64
	return true;
}
