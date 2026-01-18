#include "SpongeTile.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"

SpongeTile::SpongeTile(int_t id) : Tile(id, Material::sponge)
{
	this->tex = 48;  // Beta: Sponge.tex = 48 (Sponge.java:11)
	// Beta: Properties set in initTiles()
}

void SpongeTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Sponge.onPlace() (Sponge.java:15-26)
	// Checks for water in 2-block radius but doesn't actually remove it yet (empty loop in Beta)
	byte_t range = RANGE;
	
	for (int_t dx = x - range; dx <= x + range; dx++)
	{
		for (int_t dy = y - range; dy <= y + range; dy++)
		{
			for (int_t dz = z - range; dz <= z + range; dz++)
			{
				// Beta: if (var1.getMaterial(var6, var7, var8) == Material.water) (Sponge.java:21)
				// Note: Beta 1.2 has empty body - water absorption not fully implemented
				if (level.getMaterial(dx, dy, dz) == Material::water)
				{
					// TODO: Absorb water (not implemented in Beta 1.2)
				}
			}
		}
	}
}

void SpongeTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Sponge.onRemove() (Sponge.java:29-39)
	// Updates neighbors in 2-block radius
	byte_t range = RANGE;
	
	for (int_t dx = x - range; dx <= x + range; dx++)
	{
		for (int_t dy = y - range; dy <= y + range; dy++)
		{
			for (int_t dz = z - range; dz <= z + range; dz++)
			{
				// Beta: var1.updateNeighborsAt(var6, var7, var8, var1.getTile(var6, var7, var8)) (Sponge.java:35)
				int_t tileId = level.getTile(dx, dy, dz);
				level.updateNeighborsAt(dx, dy, dz, tileId);
			}
		}
	}
}
