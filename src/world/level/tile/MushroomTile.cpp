#include "world/level/tile/MushroomTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "java/Random.h"

// Alpha: BlockMushroom.java
MushroomTile::MushroomTile(int_t id, int_t tex) : FlowerTile(id, tex)
{
	// Alpha: BlockMushroom sets bounds (0.5 - 0.2, 0, 0.5 - 0.2, 0.5 + 0.2, 0.2 * 2, 0.5 + 0.2)
	// = (0.3, 0, 0.3, 0.7, 0.4, 0.7) (BlockMushroom.java:6-7)
	float size = 0.2f;
	setShape(0.5f - size, 0.0f, 0.5f - size, 0.5f + size, size * 2.0f, 0.5f + size);
	
	setDestroyTime(0.0f);  // Alpha: hardness 0.0F (Block.java:61-62)
}

bool MushroomTile::canThisPlantGrowOnThisBlockID(int_t blockId)
{
	// Alpha: BlockMushroom.canThisPlantGrowOnThisBlockID() returns true for any opaque block (BlockMushroom.java:10-11)
	// Block.opaqueCubeLookup[blockId] in Alpha
	return Tile::solid[blockId];  // Check if block is solid/opaque
}

bool MushroomTile::canBlockStay(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha: BlockMushroom.canBlockStay() - light <= 13 and opaque block below (BlockMushroom.java:14-15)
	int_t lightValue = level.getRawBrightness(x, y, z);
	if (lightValue > 13)
		return false;
	
	int_t belowId = level.getTile(x, y - 1, z);
	return canThisPlantGrowOnThisBlockID(belowId);
}

void MushroomTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Alpha: BlockMushroom inherits BlockFlower.updateTick() which checks canBlockStay
	if (!canBlockStay(level, x, y, z))
	{
		spawnResources(level, x, y, z, 0);
		level.setTile(x, y, z, 0);
	}
}
