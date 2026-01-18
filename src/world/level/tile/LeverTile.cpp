#include "world/level/tile/LeverTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

// Beta: LeverTile(int var1, int var2) (LeverTile.java:10-12)
LeverTile::LeverTile(int_t id, int_t tex) : Tile(id, tex, Material::decoration)
{
}

AABB *LeverTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeverTile.getAABB() - returns null (no collision) (LeverTile.java:15-17)
	return nullptr;
}

bool LeverTile::isSolidRender()
{
	// Beta: LeverTile.isSolidRender() returns false (LeverTile.java:24-26)
	return false;
}

bool LeverTile::isCubeShaped()
{
	// Beta: LeverTile.isCubeShaped() returns false (LeverTile.java:29-31)
	return false;
}

Tile::Shape LeverTile::getRenderShape()
{
	// Beta: LeverTile.getRenderShape() returns 12 (SHAPE_LEVER) (LeverTile.java:34-36)
	return SHAPE_LEVER;
}

bool LeverTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeverTile.mayPlace() - checks if can be placed on solid block (LeverTile.java:39-49)
	if (level.isSolidTile(x - 1, y, z))
	{
		return true;
	}
	else if (level.isSolidTile(x + 1, y, z))
	{
		return true;
	}
	else if (level.isSolidTile(x, y, z - 1))
	{
		return true;
	}
	else
	{
		return level.isSolidTile(x, y, z + 1) ? true : level.isSolidTile(x, y - 1, z);
	}
}

void LeverTile::setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: LeverTile.setPlacedOnFace() - sets orientation based on face (LeverTile.java:52-77)
	int_t var6 = level.getData(x, y, z);
	int_t var7 = var6 & 8;
	var6 &= 7;
	
	// Beta: Java uses int var5 (face value 0-5), C++ uses Facing enum
	int_t faceVal = static_cast<int_t>(face);
	
	if (faceVal == 1 && level.isSolidTile(x, y - 1, z))  // Beta: if (var5 == 1 && ...) var6 = 5 + random (UP face)
	{
		var6 = 5 + level.random.nextInt(2);  // Beta: var6 = 5 + var1.random.nextInt(2)
	}
	
	if (faceVal == 2 && level.isSolidTile(x, y, z + 1))  // Beta: if (var5 == 2 && ...) var6 = 4 (NORTH face)
	{
		var6 = 4;
	}
	
	if (faceVal == 3 && level.isSolidTile(x, y, z - 1))  // Beta: if (var5 == 3 && ...) var6 = 3 (SOUTH face)
	{
		var6 = 3;
	}
	
	if (faceVal == 4 && level.isSolidTile(x + 1, y, z))  // Beta: if (var5 == 4 && ...) var6 = 2 (WEST face)
	{
		var6 = 2;
	}
	
	if (faceVal == 5 && level.isSolidTile(x - 1, y, z))  // Beta: if (var5 == 5 && ...) var6 = 1 (EAST face)
	{
		var6 = 1;
	}
	
	level.setData(x, y, z, var6 + var7);
}

void LeverTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeverTile.onPlace() - sets orientation based on adjacent solid blocks (LeverTile.java:80-94)
	if (level.isSolidTile(x - 1, y, z))
	{
		level.setData(x, y, z, 1);
	}
	else if (level.isSolidTile(x + 1, y, z))
	{
		level.setData(x, y, z, 2);
	}
	else if (level.isSolidTile(x, y, z - 1))
	{
		level.setData(x, y, z, 3);
	}
	else if (level.isSolidTile(x, y, z + 1))
	{
		level.setData(x, y, z, 4);
	}
	else if (level.isSolidTile(x, y - 1, z))
	{
		level.setData(x, y, z, 5 + level.random.nextInt(2));
	}
	
	checkCanSurvive(level, x, y, z);
}

void LeverTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: LeverTile.neighborChanged() - checks if lever can survive (LeverTile.java:97-126)
	if (checkCanSurvive(level, x, y, z))
	{
		int_t var6 = level.getData(x, y, z) & 7;
		bool var7 = false;
		if (!level.isSolidTile(x - 1, y, z) && var6 == 1)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x + 1, y, z) && var6 == 2)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x, y, z - 1) && var6 == 3)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x, y, z + 1) && var6 == 4)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x, y - 1, z) && var6 == 5)
		{
			var7 = true;
		}
		
		if (var7)
		{
			spawnResources(level, x, y, z, level.getData(x, y, z));
			level.setTile(x, y, z, 0);
		}
	}
}

bool LeverTile::checkCanSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeverTile.checkCanSurvive() - checks if lever can survive (LeverTile.java:128-136)
	if (!mayPlace(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
		return false;
	}
	else
	{
		return true;
	}
}

void LeverTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: LeverTile.updateShape() - sets shape based on data (LeverTile.java:139-154)
	int_t var5 = level.getData(x, y, z) & 7;
	float var6 = 0.1875F;
	if (var5 == 1)
	{
		setShape(0.0F, 0.2F, 0.5F - var6, var6 * 2.0F, 0.8F, 0.5F + var6);
	}
	else if (var5 == 2)
	{
		setShape(1.0F - var6 * 2.0F, 0.2F, 0.5F - var6, 1.0F, 0.8F, 0.5F + var6);
	}
	else if (var5 == 3)
	{
		setShape(0.5F - var6, 0.2F, 0.0F, 0.5F + var6, 0.8F, var6 * 2.0F);
	}
	else if (var5 == 4)
	{
		setShape(0.5F - var6, 0.2F, 1.0F - var6 * 2.0F, 0.5F + var6, 0.8F, 1.0F);
	}
	else
	{
		var6 = 0.25F;
		setShape(0.5F - var6, 0.0F, 0.5F - var6, 0.5F + var6, 0.6F, 0.5F + var6);
	}
}

void LeverTile::attack(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: LeverTile.attack() - calls use() (LeverTile.java:157-159)
	use(level, x, y, z, player);
}

bool LeverTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: LeverTile.use() - toggles lever state (LeverTile.java:162-187)
	if (level.isOnline)
	{
		return true;
	}
	else
	{
		int_t var6 = level.getData(x, y, z);
		int_t var7 = var6 & 7;
		int_t var8 = 8 - (var6 & 8);
		level.setData(x, y, z, var7 + var8);
		level.setTilesDirty(x, y, z, x, y, z);
		level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.click", 0.3F, var8 > 0 ? 0.6F : 0.5F);
		level.updateNeighborsAt(x, y, z, id);
		if (var7 == 1)
		{
			level.updateNeighborsAt(x - 1, y, z, id);
		}
		else if (var7 == 2)
		{
			level.updateNeighborsAt(x + 1, y, z, id);
		}
		else if (var7 == 3)
		{
			level.updateNeighborsAt(x, y, z - 1, id);
		}
		else if (var7 == 4)
		{
			level.updateNeighborsAt(x, y, z + 1, id);
		}
		else
		{
			level.updateNeighborsAt(x, y - 1, z, id);
		}
		
		return true;
	}
}

void LeverTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeverTile.onRemove() - updates neighbors when removed (LeverTile.java:190-209)
	int_t var5 = level.getData(x, y, z);
	if ((var5 & 8) > 0)
	{
		level.updateNeighborsAt(x, y, z, id);
		int_t var6 = var5 & 7;
		if (var6 == 1)
		{
			level.updateNeighborsAt(x - 1, y, z, id);
		}
		else if (var6 == 2)
		{
			level.updateNeighborsAt(x + 1, y, z, id);
		}
		else if (var6 == 3)
		{
			level.updateNeighborsAt(x, y, z - 1, id);
		}
		else if (var6 == 4)
		{
			level.updateNeighborsAt(x, y, z + 1, id);
		}
		else
		{
			level.updateNeighborsAt(x, y - 1, z, id);
		}
	}
	
	Tile::onRemove(level, x, y, z);
}

bool LeverTile::getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: LeverTile.getSignal() - returns signal state (LeverTile.java:212-214)
	return (level.getData(x, y, z) & 8) > 0;
}

bool LeverTile::getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: LeverTile.getDirectSignal() - returns direct signal (LeverTile.java:217-233)
	int_t var6 = level.getData(x, y, z);
	if ((var6 & 8) == 0)
	{
		return false;
	}
	else
	{
		int_t var7 = var6 & 7;
		if (var7 == 5 && facing == 1)
		{
			return true;
		}
		else if (var7 == 4 && facing == 2)
		{
			return true;
		}
		else if (var7 == 3 && facing == 3)
		{
			return true;
		}
		else
		{
			return var7 == 2 && facing == 4 ? true : var7 == 1 && facing == 5;
		}
	}
}

bool LeverTile::isSignalSource()
{
	// Beta: LeverTile.isSignalSource() returns true (LeverTile.java:236-238)
	return true;
}
