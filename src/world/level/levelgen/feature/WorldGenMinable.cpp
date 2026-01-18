#include "world/level/levelgen/feature/WorldGenMinable.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/GravelTile.h"
#include "world/level/tile/ClayTile.h"
#include "java/Random.h"
#include "util/Mth.h"
#include <cmath>

// Alpha: WorldGenMinable.java
WorldGenMinable::WorldGenMinable(int_t blockId, int_t veinSize) : minableBlockId(blockId), numberOfBlocks(veinSize)
{
}

bool WorldGenMinable::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenMinable.java:14-43
	// Generate an ellipsoid ore vein
	float angle = random.nextFloat() * Mth::PI;
	double x1 = (double)((float)(x + 8) + Mth::sin(angle) * (float)numberOfBlocks / 8.0F);
	double x2 = (double)((float)(x + 8) - Mth::sin(angle) * (float)numberOfBlocks / 8.0F);
	double z1 = (double)((float)(z + 8) + Mth::cos(angle) * (float)numberOfBlocks / 8.0F);
	double z2 = (double)((float)(z + 8) - Mth::cos(angle) * (float)numberOfBlocks / 8.0F);
	double y1 = (double)(y + random.nextInt(3) + 2);
	double y2 = (double)(y + random.nextInt(3) + 2);
	
	for (int_t i = 0; i <= numberOfBlocks; ++i)
	{
		double px = x1 + (x2 - x1) * (double)i / (double)numberOfBlocks;
		double py = y1 + (y2 - y1) * (double)i / (double)numberOfBlocks;
		double pz = z1 + (z2 - z1) * (double)i / (double)numberOfBlocks;
		double radius = random.nextDouble() * (double)numberOfBlocks / 16.0;
		double radiusX = (double)(Mth::sin((float)i * Mth::PI / (float)numberOfBlocks) + 1.0F) * radius + 1.0;
		double radiusY = (double)(Mth::sin((float)i * Mth::PI / (float)numberOfBlocks) + 1.0F) * radius + 1.0;
		
		// Alpha: Replace stone blocks in ellipsoid
		for (int_t dx = (int_t)(px - radiusX / 2.0); dx <= (int_t)(px + radiusX / 2.0); ++dx)
		{
			for (int_t dy = (int_t)(py - radiusY / 2.0); dy <= (int_t)(py + radiusY / 2.0); ++dy)
			{
				for (int_t dz = (int_t)(pz - radiusX / 2.0); dz <= (int_t)(pz + radiusX / 2.0); ++dz)
				{
					double distX = ((double)dx + 0.5 - px) / (radiusX / 2.0);
					double distY = ((double)dy + 0.5 - py) / (radiusY / 2.0);
					double distZ = ((double)dz + 0.5 - pz) / (radiusX / 2.0);
					// Alpha: Replace stone for ores, or sand/gravel for clay
					int_t currentTile = level.getTile(dx, dy, dz);
					bool canReplace = false;
					if (minableBlockId == Tile::clay.id)
					{
						// Clay replaces sand or gravel (Alpha: BlockClay generation replaces sand/gravel)
						canReplace = (currentTile == Tile::sand.id || currentTile == Tile::gravel.id);
					}
					else
					{
						// Ores replace stone
						canReplace = (currentTile == Tile::rock.id);
					}
					
					if (distX * distX + distY * distY + distZ * distZ < 1.0 && canReplace)
					{
						level.setTile(dx, dy, dz, minableBlockId);
					}
				}
			}
		}
	}
	return true;
}
