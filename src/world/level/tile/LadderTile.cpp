#include "world/level/tile/LadderTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"

// Beta: LadderTile(int var1, int var2) (LadderTile.java:9-11)
LadderTile::LadderTile(int_t id, int_t tex) : Tile(id, tex, Material::decoration)
{
	// Beta: LadderTile uses Material.decoration (LadderTile.java:10)
	// Beta: LadderTile.blocksLight() returns false (LadderTile.java:59-61)
	// Beta: LadderTile.isSolidRender() returns false (LadderTile.java:64-66)
	// Ladders don't block light - ensure lightBlock is 0
	setLightBlock(0);  // Beta: isSolidRender() = false, so lightBlock should be 0
}

AABB *LadderTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LadderTile.getAABB() - collision bounding box based on facing (LadderTile.java:17-30)
	int_t data = level.getData(x, y, z);  // Beta: int var5 = var1.getData(var2, var3, var4) (LadderTile.java:18)
	float offset = 0.125f;  // Beta: float var6 = 0.125F (LadderTile.java:19)
	
	// Beta: Set shape based on facing direction (LadderTile.java:17-31)
	if (data == 2)  // Beta: if (var5 == 2) - facing south (LadderTile.java:17)
	{
		setShape(0.0f, 0.0f, 1.0f - offset, 1.0f, 1.0f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 1.0F - var6, 1.0F, 1.0F, 1.0F) (LadderTile.java:18)
	}
	
	if (data == 3)  // Beta: if (var5 == 3) - facing north (LadderTile.java:21)
	{
		setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, offset);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, var6) (LadderTile.java:22)
	}
	
	if (data == 4)  // Beta: if (var5 == 4) - facing east (LadderTile.java:25)
	{
		setShape(1.0f - offset, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);  // Beta: this.setShape(1.0F - var6, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F) (LadderTile.java:26)
	}
	
	if (data == 5)  // Beta: if (var5 == 5) - facing west (LadderTile.java:29)
	{
		setShape(0.0f, 0.0f, 0.0f, offset, 1.0f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, var6, 1.0F, 1.0F) (LadderTile.java:30)
	}
	
	// Beta: Return AABB using the shape (LadderTile.java:33)
	return Tile::getAABB(level, x, y, z);  // Beta: return super.getAABB(var1, var2, var3, var4) (LadderTile.java:33)
}

AABB *LadderTile::getTileAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LadderTile.getTileAABB() - collision bounding box for tile entities (LadderTile.java:36-57)
	int_t data = level.getData(x, y, z);  // Beta: int var5 = var1.getData(var2, var3, var4) (LadderTile.java:38)
	float offset = 0.125f;  // Beta: float var6 = 0.125F (LadderTile.java:39)
	
	// Beta: Set shape based on facing direction (LadderTile.java:40-54)
	if (data == 2)  // Beta: if (var5 == 2) - facing south (LadderTile.java:40)
	{
		setShape(0.0f, 0.0f, 1.0f - offset, 1.0f, 1.0f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 1.0F - var6, 1.0F, 1.0F, 1.0F) (LadderTile.java:41)
	}
	
	if (data == 3)  // Beta: if (var5 == 3) - facing north (LadderTile.java:44)
	{
		setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, offset);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, var6) (LadderTile.java:45)
	}
	
	if (data == 4)  // Beta: if (var5 == 4) - facing east (LadderTile.java:48)
	{
		setShape(1.0f - offset, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);  // Beta: this.setShape(1.0F - var6, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F) (LadderTile.java:49)
	}
	
	if (data == 5)  // Beta: if (var5 == 5) - facing west (LadderTile.java:52)
	{
		setShape(0.0f, 0.0f, 0.0f, offset, 1.0f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, var6, 1.0F, 1.0F) (LadderTile.java:53)
	}
	
	// Beta: Return AABB using the shape (LadderTile.java:56)
	return Tile::getTileAABB(level, x, y, z);  // Beta: return super.getTileAABB(var1, var2, var3, var4) (LadderTile.java:56)
}

Tile::Shape LadderTile::getRenderShape()
{
	// Beta: LadderTile.getRenderShape() - returns SHAPE_LADDER (LadderTile.java:32-34)
	return SHAPE_LADDER;  // Beta: return 10 (SHAPE_LADDER) (LadderTile.java:33)
}

bool LadderTile::isSolidRender()
{
	// Beta: LadderTile.isSolidRender() - returns false (non-solid) (LadderTile.java:36-38)
	return false;  // Beta: return false (LadderTile.java:37)
}

bool LadderTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LadderTile.mayPlace() - checks if can attach to adjacent solid block (LadderTile.java:79-87)
	if (level.isSolidTile(x - 1, y, z))  // Beta: if (var1.isSolidTile(var2 - 1, var3, var4)) (LadderTile.java:80)
	{
		return true;  // Beta: return true (LadderTile.java:81)
	}
	else if (level.isSolidTile(x + 1, y, z))  // Beta: else if (var1.isSolidTile(var2 + 1, var3, var4)) (LadderTile.java:82)
	{
		return true;  // Beta: return true (LadderTile.java:83)
	}
	else
	{
		// Beta: return var1.isSolidTile(var2, var3, var4 - 1) ? true : var1.isSolidTile(var2, var3, var4 + 1) (LadderTile.java:85)
		return level.isSolidTile(x, y, z - 1) ? true : level.isSolidTile(x, y, z + 1);
	}
}

void LadderTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LadderTile doesn't override onPlace() in Java - call base class
	// The orientation is set by setPlacedOnFace() instead (called from TileItem::useOn)
	Tile::onPlace(level, x, y, z);
}

void LadderTile::setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: LadderTile.setPlacedOnFace() - sets orientation based on face and adjacent blocks (LadderTile.java:90-109)
	int_t var6 = level.getData(x, y, z);  // Beta: int var6 = var1.getData(var2, var3, var4) (LadderTile.java:91)
	
	// Beta: Java uses int var5 (face value 0-5), C++ uses Facing enum
	int_t faceVal = static_cast<int_t>(face);
	
	if ((var6 == 0 || faceVal == 2) && level.isSolidTile(x, y, z + 1))  // Beta: if ((var6 == 0 || var5 == 2) && var1.isSolidTile(var2, var3, var4 + 1)) (LadderTile.java:92-93)
	{
		var6 = 2;  // Beta: var6 = 2 (LadderTile.java:93)
	}
	
	if ((var6 == 0 || faceVal == 3) && level.isSolidTile(x, y, z - 1))  // Beta: if ((var6 == 0 || var5 == 3) && var1.isSolidTile(var2, var3, var4 - 1)) (LadderTile.java:96-97)
	{
		var6 = 3;  // Beta: var6 = 3 (LadderTile.java:97)
	}
	
	if ((var6 == 0 || faceVal == 4) && level.isSolidTile(x + 1, y, z))  // Beta: if ((var6 == 0 || var5 == 4) && var1.isSolidTile(var2 + 1, var3, var4)) (LadderTile.java:100-101)
	{
		var6 = 4;  // Beta: var6 = 4 (LadderTile.java:101)
	}
	
	if ((var6 == 0 || faceVal == 5) && level.isSolidTile(x - 1, y, z))  // Beta: if ((var6 == 0 || var5 == 5) && var1.isSolidTile(var2 - 1, var3, var4)) (LadderTile.java:104-105)
	{
		var6 = 5;  // Beta: var6 = 5 (LadderTile.java:105)
	}
	
	level.setData(x, y, z, var6);  // Beta: var1.setData(var2, var3, var4, var6) (LadderTile.java:108)
}

void LadderTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: LadderTile.neighborChanged() - removes if attached block is gone (LadderTile.java:112-137)
	int_t var6 = level.getData(x, y, z);  // Beta: int var6 = var1.getData(var2, var3, var4) (LadderTile.java:113)
	bool var7 = false;  // Beta: boolean var7 = false (LadderTile.java:114)
	
	if (var6 == 2 && level.isSolidTile(x, y, z + 1))  // Beta: if (var6 == 2 && var1.isSolidTile(var2, var3, var4 + 1)) (LadderTile.java:115-116)
	{
		var7 = true;  // Beta: var7 = true (LadderTile.java:116)
	}
	
	if (var6 == 3 && level.isSolidTile(x, y, z - 1))  // Beta: if (var6 == 3 && var1.isSolidTile(var2, var3, var4 - 1)) (LadderTile.java:119-120)
	{
		var7 = true;  // Beta: var7 = true (LadderTile.java:120)
	}
	
	if (var6 == 4 && level.isSolidTile(x + 1, y, z))  // Beta: if (var6 == 4 && var1.isSolidTile(var2 + 1, var3, var4)) (LadderTile.java:123-124)
	{
		var7 = true;  // Beta: var7 = true (LadderTile.java:124)
	}
	
	if (var6 == 5 && level.isSolidTile(x - 1, y, z))  // Beta: if (var6 == 5 && var1.isSolidTile(var2 - 1, var3, var4)) (LadderTile.java:127-128)
	{
		var7 = true;  // Beta: var7 = true (LadderTile.java:128)
	}
	
	if (!var7)  // Beta: if (!var7) (LadderTile.java:131)
	{
		spawnResources(level, x, y, z, var6);  // Beta: this.spawnResources(var1, var2, var3, var4, var6) (LadderTile.java:132)
		level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0) (LadderTile.java:133)
	}
	
	Tile::neighborChanged(level, x, y, z, tile);  // Beta: super.neighborChanged(var1, var2, var3, var4, var5) (LadderTile.java:136)
}

int_t LadderTile::getResourceCount(Random &random)
{
	// Beta: LadderTile.getResourceCount() - returns 1 (LadderTile.java:140-142)
	return 1;  // Beta: return 1 (LadderTile.java:141)
}
