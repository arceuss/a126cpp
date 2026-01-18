#include "world/level/tile/SignTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/Tile.h"
#include "world/item/Items.h"
#include "world/phys/AABB.h"
#include "java/Random.h"
#include <memory>

// Beta: SignTile(int var1, Class<? extends TileEntity> var2, boolean var3) (SignTile.java:15-23)
SignTile::SignTile(int_t id, bool onGround) : Tile(id, 4, Material::wood), onGround(onGround)
{
	// Beta: SignTile extends EntityTile (EntityTile.java:8-11) - sets isEntityTile[id] = true
	Tile::isEntityTile[id] = true;
	
	// Beta: this.tex = 4 (SignTile.java:18)
	// Beta: Material.wood (SignTile.java:16)
	
	// Beta: setShape(0.5F - var4, 0.0F, 0.5F - var4, 0.5F + var4, var5, 0.5F + var4) (SignTile.java:22)
	float var4 = 0.25f;  // Beta: float var4 = 0.25F (SignTile.java:20)
	float var5 = 1.0f;   // Beta: float var5 = 1.0F (SignTile.java:21)
	setShape(0.5f - var4, 0.0f, 0.5f - var4, 0.5f + var4, var5, 0.5f + var4);
	
	// Beta: SignTile.isSolidRender() returns false (SignTile.java:74-77)
	// Signs don't block light - ensure lightBlock is 0
	setLightBlock(0);  // Beta: isSolidRender() = false, so lightBlock should be 0
}

AABB *SignTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: SignTile.getAABB() - returns null (no collision) (SignTile.java:26-28)
	return nullptr;  // Beta: return null (SignTile.java:27)
}

AABB *SignTile::getTileAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: SignTile.getTileAABB() - calls updateShape() first (SignTile.java:30-34)
	updateShape(level, x, y, z);  // Beta: this.updateShape(var1, var2, var3, var4) (SignTile.java:32)
	return Tile::getTileAABB(level, x, y, z);  // Beta: return super.getTileAABB(var1, var2, var3, var4) (SignTile.java:33)
}

void SignTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: SignTile.updateShape() - adjusts shape based on data for wall signs (SignTile.java:36-62)
	if (!onGround)  // Beta: if (!this.onGround) (SignTile.java:38)
	{
		int_t var5 = level.getData(x, y, z);  // Beta: int var5 = var1.getData(var2, var3, var4) (SignTile.java:39)
		float var6 = 0.28125f;  // Beta: float var6 = 0.28125F (SignTile.java:40)
		float var7 = 0.78125f;  // Beta: float var7 = 0.78125F (SignTile.java:41)
		float var8 = 0.0f;      // Beta: float var8 = 0.0F (SignTile.java:42)
		float var9 = 1.0f;      // Beta: float var9 = 1.0F (SignTile.java:43)
		float var10 = 0.125f;   // Beta: float var10 = 0.125F (SignTile.java:44)
		
		setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F) (SignTile.java:45)
		
		if (var5 == 2)  // Beta: if (var5 == 2) (SignTile.java:46) - facing south
		{
			setShape(var8, var6, 1.0f - var10, var9, var7, 1.0f);  // Beta: this.setShape(var8, var6, 1.0F - var10, var9, var7, 1.0F) (SignTile.java:47)
		}
		
		if (var5 == 3)  // Beta: if (var5 == 3) (SignTile.java:50) - facing north
		{
			setShape(var8, var6, 0.0f, var9, var7, var10);  // Beta: this.setShape(var8, var6, 0.0F, var9, var7, var10) (SignTile.java:51)
		}
		
		if (var5 == 4)  // Beta: if (var5 == 4) (SignTile.java:54) - facing east
		{
			setShape(1.0f - var10, var6, var8, 1.0f, var7, var9);  // Beta: this.setShape(1.0F - var10, var6, var8, 1.0F, var7, var9) (SignTile.java:55)
		}
		
		if (var5 == 5)  // Beta: if (var5 == 5) (SignTile.java:58) - facing west
		{
			setShape(0.0f, var6, var8, var10, var7, var9);  // Beta: this.setShape(0.0F, var6, var8, var10, var7, var9) (SignTile.java:59)
		}
	}
}

Tile::Shape SignTile::getRenderShape()
{
	// Beta: SignTile.getRenderShape() - returns -1 (SHAPE_INVISIBLE) (SignTile.java:64-67)
	// Signs are rendered via TileEntity renderer, not as blocks
	return SHAPE_INVISIBLE;  // Beta: return -1 (SignTile.java:66)
}

bool SignTile::isCubeShaped()
{
	// Beta: SignTile.isCubeShaped() - returns false (SignTile.java:69-72)
	return false;  // Beta: return false (SignTile.java:71)
}

bool SignTile::isSolidRender()
{
	// Beta: SignTile.isSolidRender() - returns false (SignTile.java:74-77)
	return false;  // Beta: return false (SignTile.java:76)
}

std::shared_ptr<TileEntity> SignTile::newTileEntity()
{
	// Beta: SignTile.newTileEntity() - returns SignTileEntity (SignTile.java:79-86)
	// Beta: In Java, uses reflection: return this.clas.newInstance()
	// In C++, just return make_shared<SignTileEntity>()
	return std::make_shared<SignTileEntity>();
}

int_t SignTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockSign.idDropped() returns Item.sign.shiftedIndex (BlockSign.java:76-77)
	// Beta: SignTile.getResource() - returns Item.sign.id (SignTile.java:88-91)
	return Items::sign != nullptr ? Items::sign->getShiftedIndex() : 0;
}

void SignTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: SignTile.neighborChanged() - checks if sign can survive (SignTile.java:93-126)
	bool var6 = false;  // Beta: boolean var6 = false (SignTile.java:95)
	
	if (onGround)  // Beta: if (this.onGround) (SignTile.java:96)
	{
		// Beta: if (!var1.getMaterial(var2, var3 - 1, var4).isSolid()) var6 = true (SignTile.java:97-99)
		if (!level.getMaterial(x, y - 1, z).isSolid())
		{
			var6 = true;
		}
	}
	else  // Beta: else (SignTile.java:100)
	{
		int_t var7 = level.getData(x, y, z);  // Beta: int var7 = var1.getData(var2, var3, var4) (SignTile.java:101)
		var6 = true;  // Beta: var6 = true (SignTile.java:102)
		
		// Beta: Check if attached block is still solid (SignTile.java:103-117)
		if (var7 == 2 && level.getMaterial(x, y, z + 1).isSolid())  // Beta: if (var7 == 2 && var1.getMaterial(var2, var3, var4 + 1).isSolid()) (SignTile.java:103-105)
		{
			var6 = false;  // Beta: var6 = false (SignTile.java:104)
		}
		
		if (var7 == 3 && level.getMaterial(x, y, z - 1).isSolid())  // Beta: if (var7 == 3 && var1.getMaterial(var2, var3, var4 - 1).isSolid()) (SignTile.java:107-109)
		{
			var6 = false;  // Beta: var6 = false (SignTile.java:108)
		}
		
		if (var7 == 4 && level.getMaterial(x + 1, y, z).isSolid())  // Beta: if (var7 == 4 && var1.getMaterial(var2 + 1, var3, var4).isSolid()) (SignTile.java:111-113)
		{
			var6 = false;  // Beta: var6 = false (SignTile.java:112)
		}
		
		if (var7 == 5 && level.getMaterial(x - 1, y, z).isSolid())  // Beta: if (var7 == 5 && var1.getMaterial(var2 - 1, var3, var4).isSolid()) (SignTile.java:115-117)
		{
			var6 = false;  // Beta: var6 = false (SignTile.java:116)
		}
	}
	
	if (var6)  // Beta: if (var6) (SignTile.java:120)
	{
		// Beta: Remove sign if it can't survive (SignTile.java:121-122)
		spawnResources(level, x, y, z, level.getData(x, y, z));  // Beta: this.spawnResources(var1, var2, var3, var4, var1.getData(var2, var3, var4)) (SignTile.java:121)
		level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0) (SignTile.java:122)
	}
	
	Tile::neighborChanged(level, x, y, z, tile);  // Beta: super.neighborChanged(var1, var2, var3, var4, var5) (SignTile.java:125)
}

void SignTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: EntityTile.onPlace() - creates TileEntity (EntityTile.java:19-22)
	Tile::onPlace(level, x, y, z);  // Beta: super.onPlace(var1, var2, var3, var4) (EntityTile.java:20)
	level.setTileEntity(x, y, z, newTileEntity());  // Beta: var1.setTileEntity(var2, var3, var4, this.newTileEntity()) (EntityTile.java:21)
}

void SignTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: EntityTile.onRemove() - removes TileEntity (EntityTile.java:24-28)
	Tile::onRemove(level, x, y, z);  // Beta: super.onRemove(var1, var2, var3, var4) (EntityTile.java:25)
	level.removeTileEntity(x, y, z);  // Beta: var1.removeTileEntity(var2, var3, var4) (EntityTile.java:26)
}
