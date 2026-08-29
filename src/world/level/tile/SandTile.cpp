#include "world/level/tile/SandTile.h"

#include <memory>

#include "world/entity/item/FallingTile.h"
#include "world/level/Level.h"
#include "world/level/tile/FireTile.h"
#include "world/level/material/LiquidMaterial.h"

bool SandTile::fallInstantly = false;

SandTile::SandTile(int_t id, int_t tex) : Tile(id, tex, Material::sand)
{
}

void SandTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha BlockSand.onBlockAdded (BlockSand.java:20-23).
	level.scheduleBlockUpdate(x, y, z, id);
}

void SandTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Alpha BlockSand.onNeighborBlockChange (BlockSand.java:25-28).
	level.scheduleBlockUpdate(x, y, z, id);
}

void SandTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	(void)random;
	tryToFall(level, x, y, z);
}

void SandTile::tryToFall(Level &level, int_t x, int_t y, int_t z)
{
	// Preserve Alpha's condition and evaluation order exactly
	// (BlockSand.java:35-45).
	if (canFallBelow(level, x, y - 1, z) && y >= 0)
	{
		std::shared_ptr<FallingTile> falling = std::make_shared<FallingTile>(
			level, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
			static_cast<double>(z) + 0.5, id);
		if (fallInstantly)
		{
			while (!falling->removed)
				falling->tick();
		}
		else
		{
			level.addEntity(falling);
		}
	}
}

int_t SandTile::getTickDelay()
{
	return 3;
}

bool SandTile::canFallBelow(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha BlockSand.canFallBelow (BlockSand.java:53-63).
	const int_t tile = level.getTile(x, y, z);
	if (tile == 0)
		return true;
	if (tile == Tile::fire.id)
		return true;
	const Material &material = Tile::tiles[tile]->material;
	return material == Material::water ? true : material == Material::lava;
}
