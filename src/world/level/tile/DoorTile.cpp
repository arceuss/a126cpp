#include "world/level/tile/DoorTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

// Beta: DoorTile(int var1, Material var2) (DoorTile.java:14-24)
DoorTile::DoorTile(int_t id, const Material &material) : Tile(id, material)
{
	tex = 97;  // Beta: this.tex = 97 (DoorTile.java:16)
	if (material == Material::metal)
	{
		tex++;  // Beta: this.tex++ (DoorTile.java:18)
	}
	
	float var3 = 0.5F;
	float var4 = 1.0F;
	Tile::setShape(0.5F - var3, 0.0F, 0.5F - var3, 0.5F + var3, var4, 0.5F + var3);  // Beta: DoorTile.java:23
	
	// Beta: DoorTile.blocksLight() returns false (DoorTile.java:47-49)
	// Doors don't block light - ensure lightBlock is 0
	setLightBlock(0);  // Beta: isSolidRender() = false, so lightBlock should be 0
}

int_t DoorTile::getTexture(Facing face, int_t data)
{
	// Beta: DoorTile.getTexture() - returns texture based on face and data (DoorTile.java:27-45)
	int_t faceVal = static_cast<int_t>(face);
	if (faceVal != 0 && faceVal != 1)  // Beta: if (var1 != 0 && var1 != 1) (DOWN/UP)
	{
		int_t var3 = getDir(data);
		if ((var3 == 0 || var3 == 2) ^ (faceVal <= 3))  // Beta: if ((var3 == 0 || var3 == 2) ^ var1 <= 3)
		{
			return tex;  // Beta: return this.tex (DoorTile.java:31)
		}
		else
		{
			int_t var4 = var3 / 2 + (faceVal & 1 ^ var3);
			var4 += (data & 4) / 4;
			int_t var5 = tex - (data & 8) * 2;
			if ((var4 & 1) != 0)
			{
				var5 = -var5;
			}
			return var5;  // Beta: return var5 (DoorTile.java:40)
		}
	}
	else
	{
		return tex;  // Beta: return this.tex (DoorTile.java:43)
	}
}

bool DoorTile::isSolidRender()
{
	// Beta: DoorTile.isSolidRender() returns false (DoorTile.java:52-54)
	return false;
}

bool DoorTile::isCubeShaped()
{
	// Beta: DoorTile.isCubeShaped() returns false (DoorTile.java:57-59)
	return false;
}

Tile::Shape DoorTile::getRenderShape()
{
	// Beta: DoorTile.getRenderShape() returns 7 (SHAPE_DOOR) (DoorTile.java:62-64)
	return SHAPE_DOOR;
}

AABB *DoorTile::getTileAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: DoorTile.getTileAABB() - updates shape and returns AABB (DoorTile.java:67-70)
	updateShape(level, x, y, z);
	return Tile::getTileAABB(level, x, y, z);
}

AABB *DoorTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: DoorTile.getAABB() - updates shape and returns AABB (DoorTile.java:73-76)
	updateShape(level, x, y, z);
	return Tile::getAABB(level, x, y, z);
}

void DoorTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: DoorTile.updateShape() - sets shape based on direction (DoorTile.java:79-81)
	int_t dir = getDir(level.getData(x, y, z));
	setShape(dir);
}

void DoorTile::setShape(int_t dir)
{
	// Beta: DoorTile.setShape() - sets shape based on direction (DoorTile.java:83-101)
	float var2 = 0.1875F;
	Tile::setShape(0.0F, 0.0F, 0.0F, 1.0F, 2.0F, 1.0F);  // Beta: DoorTile.java:85
	
	if (dir == 0)
	{
		Tile::setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, var2);  // Beta: DoorTile.java:87
	}
	
	if (dir == 1)
	{
		Tile::setShape(1.0F - var2, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);  // Beta: DoorTile.java:90-91
	}
	
	if (dir == 2)
	{
		Tile::setShape(0.0F, 0.0F, 1.0F - var2, 1.0F, 1.0F, 1.0F);  // Beta: DoorTile.java:94-95
	}
	
	if (dir == 3)
	{
		Tile::setShape(0.0F, 0.0F, 0.0F, var2, 1.0F, 1.0F);  // Beta: DoorTile.java:98-99
	}
}

void DoorTile::attack(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: DoorTile.attack() - calls use() (DoorTile.java:104-106)
	use(level, x, y, z, player);
}

bool DoorTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: DoorTile.use() - toggles door state (DoorTile.java:109-136)
	if (material == Material::metal)
	{
		return true;  // Beta: return true (iron doors can't be opened manually) (DoorTile.java:111)
	}
	else
	{
		int_t var6 = level.getData(x, y, z);
		if ((var6 & 8) != 0)  // Beta: if ((var6 & 8) != 0) (upper half)
		{
			if (level.getTile(x, y - 1, z) == id)  // Beta: if (var1.getTile(var2, var3 - 1, var4) == this.id)
			{
				use(level, x, y - 1, z, player);  // Beta: this.use(var1, var2, var3 - 1, var4, var5) (DoorTile.java:116)
			}
			return true;  // Beta: return true (DoorTile.java:119)
		}
		else
		{
			if (level.getTile(x, y + 1, z) == id)  // Beta: if (var1.getTile(var2, var3 + 1, var4) == this.id)
			{
				level.setData(x, y + 1, z, (var6 ^ 4) + 8);  // Beta: var1.setData(var2, var3 + 1, var4, (var6 ^ 4) + 8) (DoorTile.java:122)
			}
			level.setData(x, y, z, var6 ^ 4);  // Beta: var1.setData(var2, var3, var4, var6 ^ 4) (DoorTile.java:125)
			level.setTilesDirty(x, y - 1, z, x, y, z);  // Beta: var1.setTilesDirty(var2, var3 - 1, var4, var2, var3, var4) (DoorTile.java:126)
			
			// Beta: Random door sound (DoorTile.java:127-131)
			if (level.random.nextFloat() < 0.5f)  // Beta: if (Math.random() < 0.5)
			{
				level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.door_open", 1.0F, level.random.nextFloat() * 0.1F + 0.9F);
			}
			else
			{
				level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.door_close", 1.0F, level.random.nextFloat() * 0.1F + 0.9F);
			}
			
			return true;  // Beta: return true (DoorTile.java:133)
		}
	}
}

void DoorTile::setOpen(Level &level, int_t x, int_t y, int_t z, bool open)
{
	// Beta: DoorTile.setOpen() - sets door open/closed state (DoorTile.java:138-160)
	int_t var6 = level.getData(x, y, z);
	if ((var6 & 8) != 0)  // Beta: if ((var6 & 8) != 0) (upper half)
	{
		if (level.getTile(x, y - 1, z) == id)  // Beta: if (var1.getTile(var2, var3 - 1, var4) == this.id)
		{
			setOpen(level, x, y - 1, z, open);  // Beta: this.setOpen(var1, var2, var3 - 1, var4, var5) (DoorTile.java:142)
		}
	}
	else
	{
		bool var7 = (level.getData(x, y, z) & 4) > 0;  // Beta: boolean var7 = (var1.getData(var2, var3, var4) & 4) > 0 (DoorTile.java:145)
		if (var7 != open)  // Beta: if (var7 != var5) (DoorTile.java:146)
		{
			if (level.getTile(x, y + 1, z) == id)  // Beta: if (var1.getTile(var2, var3 + 1, var4) == this.id)
			{
				level.setData(x, y + 1, z, (var6 ^ 4) + 8);  // Beta: var1.setData(var2, var3 + 1, var4, (var6 ^ 4) + 8) (DoorTile.java:148)
			}
			level.setData(x, y, z, var6 ^ 4);  // Beta: var1.setData(var2, var3, var4, var6 ^ 4) (DoorTile.java:151)
			level.setTilesDirty(x, y - 1, z, x, y, z);  // Beta: var1.setTilesDirty(var2, var3 - 1, var4, var2, var3, var4) (DoorTile.java:152)
			
			// Beta: Random door sound (DoorTile.java:153-157)
			if (level.random.nextFloat() < 0.5f)  // Beta: if (Math.random() < 0.5)
			{
				level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.door_open", 1.0F, level.random.nextFloat() * 0.1F + 0.9F);
			}
			else
			{
				level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.door_close", 1.0F, level.random.nextFloat() * 0.1F + 0.9F);
			}
		}
	}
}

void DoorTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: DoorTile.neighborChanged() - handles door updates (DoorTile.java:163-195)
	int_t var6 = level.getData(x, y, z);
	if ((var6 & 8) != 0)  // Beta: if ((var6 & 8) != 0) (upper half)
	{
		if (level.getTile(x, y - 1, z) != id)  // Beta: if (var1.getTile(var2, var3 - 1, var4) != this.id)
		{
			level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0) (DoorTile.java:167)
		}
		
		if (tile > 0 && Tile::tiles[tile]->isSignalSource())  // Beta: if (var5 > 0 && Tile.tiles[var5].isSignalSource()) (DoorTile.java:170)
		{
			neighborChanged(level, x, y - 1, z, tile);  // Beta: this.neighborChanged(var1, var2, var3 - 1, var4, var5) (DoorTile.java:171)
		}
	}
	else
	{
		bool var7 = false;
		if (level.getTile(x, y + 1, z) != id)  // Beta: if (var1.getTile(var2, var3 + 1, var4) != this.id) (DoorTile.java:175)
		{
			level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0) (DoorTile.java:176)
			var7 = true;  // Beta: var7 = true (DoorTile.java:177)
		}
		
		if (!level.isSolidTile(x, y - 1, z))  // Beta: if (!var1.isSolidTile(var2, var3 - 1, var4)) (DoorTile.java:180)
		{
			level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0) (DoorTile.java:181)
			var7 = true;  // Beta: var7 = true (DoorTile.java:182)
			if (level.getTile(x, y + 1, z) == id)  // Beta: if (var1.getTile(var2, var3 + 1, var4) == this.id) (DoorTile.java:183)
			{
				level.setTile(x, y + 1, z, 0);  // Beta: var1.setTile(var2, var3 + 1, var4, 0) (DoorTile.java:184)
			}
		}
		
		if (var7)  // Beta: if (var7) (DoorTile.java:188)
		{
			spawnResources(level, x, y, z, var6);  // Beta: this.spawnResources(var1, var2, var3, var4, var6) (DoorTile.java:189)
		}
		else if (tile > 0 && Tile::tiles[tile]->isSignalSource())  // Beta: else if (var5 > 0 && Tile.tiles[var5].isSignalSource()) (DoorTile.java:190)
		{
			bool var8 = level.hasNeighborSignal(x, y, z) || level.hasNeighborSignal(x, y + 1, z);  // Beta: boolean var8 = var1.hasNeighborSignal(var2, var3, var4) || var1.hasNeighborSignal(var2, var3 + 1, var4) (DoorTile.java:191)
			setOpen(level, x, y, z, var8);  // Beta: this.setOpen(var1, var2, var3, var4, var8) (DoorTile.java:192)
		}
	}
}

int_t DoorTile::getResource(int_t data, Random &random)
{
	// Beta: DoorTile.getResource() - returns door item (DoorTile.java:198-204)
	if ((data & 8) != 0)  // Beta: if ((var1 & 8) != 0) (upper half)
	{
		return 0;  // Beta: return 0 (DoorTile.java:200)
	}
	else
	{
		// Beta: return this.material == Material.metal ? Item.door_iron.id : Item.door_wood.id (DoorTile.java:202)
		// TODO: Return Item.door_iron.id or Item.door_wood.id once items are implemented
		// For now, return placeholder (door_wood = 68, door_iron = 74 in Beta 1.2 Item.java:88, 94)
		return material == Material::metal ? 74 + 256 : 68 + 256;  // shiftedIndex = baseID + 256
	}
}

HitResult DoorTile::clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to)
{
	// Beta: DoorTile.clip() - updates shape and clips (DoorTile.java:207-210)
	updateShape(level, x, y, z);
	return Tile::clip(level, x, y, z, from, to);
}

int_t DoorTile::getDir(int_t data)
{
	// Beta: DoorTile.getDir() - gets door direction from data (DoorTile.java:212-214)
	return (data & 4) == 0 ? (data - 1) & 3 : data & 3;
}

bool DoorTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: DoorTile.mayPlace() - checks if door can be placed (DoorTile.java:217-221)
	return y >= 127
		? false
		: level.isSolidTile(x, y - 1, z) && Tile::mayPlace(level, x, y, z) && Tile::mayPlace(level, x, y + 1, z);
}
