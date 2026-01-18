#include "world/level/tile/SnowBlockTile.h"
#include "world/level/Level.h"
#include "world/level/LightLayer.h"
#include "world/level/material/Material.h"
#include "world/item/Items.h"
#include "java/Random.h"

// Beta: Tile.snow = new SnowTile(80, 66).setDestroyTime(0.2F).setSoundType(SOUND_CLOTH)
SnowBlockTile::SnowBlockTile(int_t id, int_t tex) : Tile(id, tex, Material::snow)
{
	setDestroyTime(0.2f);
	setTicking(true);
	// Sound type is handled by Material::snow
}

int_t SnowBlockTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockSnowBlock.idDropped() returns Item.snowball.shiftedIndex (BlockSnowBlock.java:11-12)
	// Beta: SnowTile.getResource() returns Item.snowBall.id (SnowTile.java:16-18)
	return Items::snowball != nullptr ? Items::snowball->getShiftedIndex() : 0;
}

int_t SnowBlockTile::getResourceCount(Random &random)
{
	// Beta: SnowTile.getResourceCount() returns 4 (SnowTile.java:20-23)
	return 4;
}

void SnowBlockTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: SnowTile.tick() - melts if light > 11 (SnowTile.java:26-31)
	if (level.getBrightness(LightLayer::Block, x, y, z) > 11)
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
	}
}
