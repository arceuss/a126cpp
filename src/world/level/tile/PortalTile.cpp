#include "world/level/tile/PortalTile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/ObsidianTile.h"
#include "world/level/tile/FireTile.h"
#include "world/entity/Entity.h"
#include "world/phys/AABB.h"
#include "java/Random.h"
#include "java/String.h"
#include "Facing.h"

// Beta: PortalTile.java
PortalTile::PortalTile(int_t id, int_t texture) : TransparentTile(id, texture, Material::portal, false)
{
	// Beta: Properties set in initTiles()
}

AABB *PortalTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: PortalTile.getAABB() (PortalTile.java:16-18)
	return nullptr;  // No collision box
}

void PortalTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: PortalTile.updateShape() (PortalTile.java:21-31)
	if (level.getTile(x - 1, y, z) != id && level.getTile(x + 1, y, z) != id)
	{
		// Portal extends in Z direction
		float var7 = 0.125F;
		float var8 = 0.5F;
		setShape(0.5F - var7, 0.0F, 0.5F - var8, 0.5F + var7, 1.0F, 0.5F + var8);
	}
	else
	{
		// Portal extends in X direction
		float var5 = 0.5F;
		float var6 = 0.125F;
		setShape(0.5F - var5, 0.0F, 0.5F - var6, 0.5F + var5, 1.0F, 0.5F + var6);
	}
}

bool PortalTile::isCubeShaped()
{
	// Beta: PortalTile.isCubeShaped() (PortalTile.java:39-41)
	return false;
}

void PortalTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: PortalTile.neighborChanged() (PortalTile.java:93-129)
	byte_t var6 = 0;
	byte_t var7 = 1;
	if (level.getTile(x - 1, y, z) == id || level.getTile(x + 1, y, z) == id)
	{
		var6 = 1;
		var7 = 0;
	}
	
	int_t var8 = y;
	while (level.getTile(x, var8 - 1, z) == id)
	{
		var8--;
	}
	
	if (level.getTile(x, var8 - 1, z) != Tile::obsidian.id)
	{
		level.setTile(x, y, z, 0);
	}
	else
	{
		int_t var9 = 1;
		while (var9 < 4 && level.getTile(x, var8 + var9, z) == id)
		{
			var9++;
		}
		
		if (var9 == 3 && level.getTile(x, var8 + var9, z) == Tile::obsidian.id)
		{
			bool var10 = level.getTile(x - 1, y, z) == id || level.getTile(x + 1, y, z) == id;
			bool var11 = level.getTile(x, y, z - 1) == id || level.getTile(x, y, z + 1) == id;
			if (var10 && var11)
			{
				level.setTile(x, y, z, 0);
			}
			else if ((level.getTile(x + var6, y, z + var7) != Tile::obsidian.id || level.getTile(x - var6, y, z - var7) != id)
				&& (level.getTile(x - var6, y, z - var7) != Tile::obsidian.id || level.getTile(x + var6, y, z + var7) != id))
			{
				level.setTile(x, y, z, 0);
			}
		}
		else
		{
			level.setTile(x, y, z, 0);
		}
	}
}

bool PortalTile::shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: PortalTile.shouldRenderFace() (PortalTile.java:132-134)
	return true;
}

int_t PortalTile::getResourceCount(Random &random)
{
	// Beta: PortalTile.getResourceCount() (PortalTile.java:137-139)
	return 0;
}

int_t PortalTile::getRenderLayer()
{
	// Beta: PortalTile.getRenderLayer() (PortalTile.java:142-144)
	return 1;
}

void PortalTile::entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: PortalTile.entityInside() (PortalTile.java:147-151)
	if (!level.isOnline)
	{
		entity.handleInsidePortal();  // Beta: var5.handleInsidePortal() (PortalTile.java:149)
	}
}

void PortalTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: PortalTile.animateTick() (PortalTile.java:154-180)
	// Note: FX is already ported, so particles should work
	if (random.nextInt(100) == 0)
	{
		level.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, jstring(u"portal.portal"), 1.0F, random.nextFloat() * 0.4F + 0.8F);
	}
	
	for (int_t var6 = 0; var6 < 4; var6++)
	{
		double var7 = (double)x + random.nextFloat();
		double var9 = (double)y + random.nextFloat();
		double var11 = (double)z + random.nextFloat();
		double var13 = 0.0;
		double var15 = 0.0;
		double var17 = 0.0;
		int_t var19 = random.nextInt(2) * 2 - 1;
		var13 = (random.nextFloat() - 0.5) * 0.5;
		var15 = (random.nextFloat() - 0.5) * 0.5;
		var17 = (random.nextFloat() - 0.5) * 0.5;
		if (level.getTile(x - 1, y, z) != id && level.getTile(x + 1, y, z) != id)
		{
			var7 = (double)x + 0.5 + 0.25 * var19;
			var13 = random.nextFloat() * 2.0F * var19;
		}
		else
		{
			var11 = (double)z + 0.5 + 0.25 * var19;
			var17 = random.nextFloat() * 2.0F * var19;
		}
		
		level.addParticle(jstring(u"portal"), var7, var9, var11, var13, var15, var17);
	}
}

bool PortalTile::trySpawnPortal(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: PortalTile.trySpawnPortal() (PortalTile.java:43-90)
	byte_t var5 = 0;
	byte_t var6 = 0;
	if (level.getTile(x - 1, y, z) == Tile::obsidian.id || level.getTile(x + 1, y, z) == Tile::obsidian.id)
	{
		var5 = 1;
	}
	
	if (level.getTile(x, y, z - 1) == Tile::obsidian.id || level.getTile(x, y, z + 1) == Tile::obsidian.id)
	{
		var6 = 1;
	}
	
	// Beta: System.out.println(var5 + ", " + var6) (PortalTile.java:54) - debug output
	// std::cout << (int)var5 << ", " << (int)var6 << '\n';
	
	if (var5 == var6)
	{
		return false;
	}
	else
	{
		if (level.getTile(x - var5, y, z - var6) == 0)
		{
			x -= var5;
			z -= var6;
		}
		
		for (int_t var7 = -1; var7 <= 2; var7++)
		{
			for (int_t var8 = -1; var8 <= 3; var8++)
			{
				bool var9 = var7 == -1 || var7 == 2 || var8 == -1 || var8 == 3;
				if (var7 != -1 && var7 != 2 || var8 != -1 && var8 != 3)
				{
					int_t var10 = level.getTile(x + var5 * var7, y + var8, z + var6 * var7);
					if (var9)
					{
						if (var10 != Tile::obsidian.id)
						{
							return false;
						}
					}
					else if (var10 != 0 && var10 != Tile::fire.id)
					{
						return false;
					}
				}
			}
		}
		
		// Beta: Set noNeighborUpdate flag before setting tiles (PortalTile.java:79)
		level.noNeighborUpdate = true;
		
		// Beta: Set portal tiles (PortalTile.java:81-85)
		for (int_t var11 = 0; var11 < 2; var11++)
		{
			for (int_t var12 = 0; var12 < 3; var12++)
			{
				level.setTile(x + var5 * var11, y + var12, z + var6 * var11, id);
			}
		}
		
		// Beta: Reset noNeighborUpdate flag after setting tiles (PortalTile.java:87)
		level.noNeighborUpdate = false;
		
		return true;
	}
}
