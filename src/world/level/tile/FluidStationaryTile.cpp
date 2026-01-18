#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"

// Beta 1.2: LiquidTileStatic.java - ported 1:1
FluidStationaryTile::FluidStationaryTile(int_t id, const LiquidMaterial &material)
	: FluidTile(id, material)
{
	// Beta: LiquidTileStatic constructor (LiquidTileStatic.java:8-14)
	setTicking(false);
	if (&material == &Material::lava)
	{
		setTicking(true);
	}
}

// Beta: LiquidTileStatic.neighborChanged() converts to dynamic (LiquidTileStatic.java:16-22)
void FluidStationaryTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	FluidTile::neighborChanged(level, x, y, z, tile);
	if (level.getTile(x, y, z) == id)
	{
		setDynamic(level, x, y, z);
	}
}

// Beta: LiquidTileStatic.setDynamic() converts to dynamic block (LiquidTileStatic.java:24-31)
void FluidStationaryTile::setDynamic(Level &level, int_t x, int_t y, int_t z)
{
	int_t data = level.getData(x, y, z);
	level.noNeighborUpdate = true;
	level.setTileAndDataNoUpdate(x, y, z, id - 1, data);
	level.setTilesDirty(x, y, z, x, y, z);
	level.addToTickNextTick(x, y, z, id - 1);
	level.noNeighborUpdate = false;
}

// Beta: LiquidTileStatic.tick() - lava can spread fire (LiquidTileStatic.java:33-58)
void FluidStationaryTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	if (&fluidMaterial == &Material::lava)
	{
		int_t attempts = random.nextInt(3);
		
		for (int_t i = 0; i < attempts; ++i)
		{
			x += random.nextInt(3) - 1;
			y++;
			z += random.nextInt(3) - 1;
			int_t blockID = level.getTile(x, y, z);
			if (blockID == 0)
			{
				if (isFlammable(level, x - 1, y, z) ||
				    isFlammable(level, x + 1, y, z) ||
				    isFlammable(level, x, y, z - 1) ||
				    isFlammable(level, x, y, z + 1) ||
				    isFlammable(level, x, y - 1, z) ||
				    isFlammable(level, x, y + 1, z))
				{
					// Beta: Tile.fire.id (ID 51)
					level.setTile(x, y, z, 51);
					return;
				}
			}
			else if (Tile::tiles[blockID] != nullptr && Tile::tiles[blockID]->material.blocksMotion())
			{
				return;
			}
		}
	}
}

// Beta: LiquidTileStatic.isFlammable() checks if block is flammable (LiquidTileStatic.java:60-62)
bool FluidStationaryTile::isFlammable(Level &level, int_t x, int_t y, int_t z)
{
	const Material &mat = level.getMaterial(x, y, z);
	return const_cast<Material&>(mat).isFlammable();
}
