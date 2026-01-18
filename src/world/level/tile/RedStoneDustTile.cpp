#include "world/level/tile/RedStoneDustTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"
#include "world/item/Item.h"
#include "world/item/Items.h"
#include "java/Random.h"

// Beta: RedStoneDustTile(int var1, int var2) (RedStoneDustTile.java:18-21)
RedStoneDustTile::RedStoneDustTile(int_t id, int_t tex) : Tile(id, tex, Material::decoration), shouldSignal(true)
{
	// Beta: RedStoneDustTile uses Material.decoration (RedStoneDustTile.java:19)
	// Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.0625F, 1.0F) (RedStoneDustTile.java:20)
	Tile::setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.0625F, 1.0F);
}

// Beta: RedStoneDustTile.getTexture() - returns texture based on data (RedStoneDustTile.java:23-26)
int_t RedStoneDustTile::getTexture(Facing face, int_t data)
{
	// Beta: return this.tex + (var2 > 0 ? 16 : 0) (RedStoneDustTile.java:25)
	return tex + (data > 0 ? 16 : 0);
}

// Beta: RedStoneDustTile.getAABB() - returns null (no collision) (RedStoneDustTile.java:28-31)
AABB *RedStoneDustTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	return nullptr;  // Beta: return null (RedStoneDustTile.java:30)
}

// Beta: RedStoneDustTile.isSolidRender() - returns false (RedStoneDustTile.java:33-36)
bool RedStoneDustTile::isSolidRender()
{
	return false;  // Beta: return false (RedStoneDustTile.java:35)
}

// Beta: RedStoneDustTile.isCubeShaped() - returns false (RedStoneDustTile.java:38-41)
bool RedStoneDustTile::isCubeShaped()
{
	return false;  // Beta: return false (RedStoneDustTile.java:40)
}

// Beta: RedStoneDustTile.getRenderShape() - returns 5 (SHAPE_RED_DUST) (RedStoneDustTile.java:43-46)
Tile::Shape RedStoneDustTile::getRenderShape()
{
	return SHAPE_RED_DUST;  // Beta: return 5 (RedStoneDustTile.java:45)
}

// Beta: RedStoneDustTile.mayPlace() - checks if can be placed on solid block (RedStoneDustTile.java:48-51)
bool RedStoneDustTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: return var1.isSolidTile(var2, var3 - 1, var4) (RedStoneDustTile.java:50)
	return level.isSolidTile(x, y - 1, z);
}

// Beta: RedStoneDustTile.updatePowerStrength() - updates power strength (RedStoneDustTile.java:53-62)
void RedStoneDustTile::updatePowerStrength(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: this.updatePowerStrength(var1, var2, var3, var4, var2, var3, var4) (RedStoneDustTile.java:54)
	updatePowerStrength(level, x, y, z, x, y, z);
	// Beta: ArrayList var5 = new ArrayList<>(this.toUpdate) (RedStoneDustTile.java:55)
	std::vector<TilePos> toUpdateList(toUpdate.begin(), toUpdate.end());
	// Beta: this.toUpdate.clear() (RedStoneDustTile.java:56)
	toUpdate.clear();

	// Beta: for (int var6 = 0; var6 < var5.size(); var6++) (RedStoneDustTile.java:58)
	for (size_t i = 0; i < toUpdateList.size(); i++)
	{
		TilePos &pos = toUpdateList[i];  // Beta: TilePos var7 = (TilePos)var5.get(var6) (RedStoneDustTile.java:59)
		// Beta: var1.updateNeighborsAt(var7.x, var7.y, var7.z, this.id) (RedStoneDustTile.java:60)
		level.updateNeighborsAt(pos.x, pos.y, pos.z, id);
	}
}

// Beta: RedStoneDustTile.updatePowerStrength() - recursive power update (RedStoneDustTile.java:64-174)
void RedStoneDustTile::updatePowerStrength(Level &level, int_t x, int_t y, int_t z, int_t fromX, int_t fromY, int_t fromZ)
{
	// Beta: int var8 = var1.getData(var2, var3, var4) (RedStoneDustTile.java:65)
	int_t currentData = level.getData(x, y, z);
	// Beta: int var9 = 0 (RedStoneDustTile.java:66)
	int_t newPower = 0;
	// Beta: this.shouldSignal = false (RedStoneDustTile.java:67)
	shouldSignal = false;
	// Beta: boolean var10 = var1.hasNeighborSignal(var2, var3, var4) (RedStoneDustTile.java:68)
	bool hasNeighbor = level.hasNeighborSignal(x, y, z);
	// Beta: this.shouldSignal = true (RedStoneDustTile.java:69)
	shouldSignal = true;
	
	// Beta: if (var10) (RedStoneDustTile.java:70)
	if (hasNeighbor)
	{
		// Beta: var9 = 15 (RedStoneDustTile.java:71)
		newPower = 15;
	}
	else
	{
		// Beta: for (int var11 = 0; var11 < 4; var11++) (RedStoneDustTile.java:73)
		for (int_t dir = 0; dir < 4; dir++)
		{
			int_t checkX = x;  // Beta: int var12 = var2 (RedStoneDustTile.java:74)
			int_t checkZ = z;  // Beta: int var13 = var4 (RedStoneDustTile.java:75)
			
			// Beta: if (var11 == 0) { var12 = var2 - 1; } (RedStoneDustTile.java:76-78)
			if (dir == 0)
				checkX = x - 1;
			// Beta: if (var11 == 1) { var12++; } (RedStoneDustTile.java:80-82)
			if (dir == 1)
				checkX++;
			// Beta: if (var11 == 2) { var13 = var4 - 1; } (RedStoneDustTile.java:84-86)
			if (dir == 2)
				checkZ = z - 1;
			// Beta: if (var11 == 3) { var13++; } (RedStoneDustTile.java:88-90)
			if (dir == 3)
				checkZ++;

			// Beta: if (var12 != var5 || var3 != var6 || var13 != var7) (RedStoneDustTile.java:92)
			if (checkX != fromX || y != fromY || checkZ != fromZ)
			{
				// Beta: var9 = this.checkTarget(var1, var12, var3, var13, var9) (RedStoneDustTile.java:93)
				newPower = checkTarget(level, checkX, y, checkZ, newPower);
			}

			// Beta: if (var1.isSolidTile(var12, var3, var13) && !var1.isSolidTile(var2, var3 + 1, var4)) (RedStoneDustTile.java:96)
			if (level.isSolidTile(checkX, y, checkZ) && !level.isSolidTile(x, y + 1, z))
			{
				// Beta: if (var12 != var5 || var3 + 1 != var6 || var13 != var7) (RedStoneDustTile.java:97)
				if (checkX != fromX || y + 1 != fromY || checkZ != fromZ)
				{
					// Beta: var9 = this.checkTarget(var1, var12, var3 + 1, var13, var9) (RedStoneDustTile.java:98)
					newPower = checkTarget(level, checkX, y + 1, checkZ, newPower);
				}
			}
			// Beta: else if (!var1.isSolidTile(var12, var3, var13) && (var12 != var5 || var3 - 1 != var6 || var13 != var7)) (RedStoneDustTile.java:100)
			else if (!level.isSolidTile(checkX, y, checkZ) && (checkX != fromX || y - 1 != fromY || checkZ != fromZ))
			{
				// Beta: var9 = this.checkTarget(var1, var12, var3 - 1, var13, var9) (RedStoneDustTile.java:101)
				newPower = checkTarget(level, checkX, y - 1, checkZ, newPower);
			}
		}

		// Beta: if (var9 > 0) { var9--; } else { var9 = 0; } (RedStoneDustTile.java:105-109)
		if (newPower > 0)
			newPower--;
		else
			newPower = 0;
	}

	// Beta: if (var8 != var9) (RedStoneDustTile.java:112)
	if (currentData != newPower)
	{
		// Beta: var1.noNeighborUpdate = true (RedStoneDustTile.java:113)
		level.noNeighborUpdate = true;
		// Beta: var1.setData(var2, var3, var4, var9) (RedStoneDustTile.java:114)
		level.setData(x, y, z, newPower);
		// Beta: var1.setTilesDirty(var2, var3, var4, var2, var3, var4) (RedStoneDustTile.java:115)
		level.setTilesDirty(x, y, z, x, y, z);
		// Beta: var1.noNeighborUpdate = false (RedStoneDustTile.java:116)
		level.noNeighborUpdate = false;

		// Beta: for (int var18 = 0; var18 < 4; var18++) (RedStoneDustTile.java:118)
		for (int_t dir = 0; dir < 4; dir++)
		{
			int_t checkX = x;  // Beta: int var19 = var2 (RedStoneDustTile.java:119)
			int_t checkZ = z;  // Beta: int var20 = var4 (RedStoneDustTile.java:120)
			int_t checkY = y - 1;  // Beta: int var14 = var3 - 1 (RedStoneDustTile.java:121)
			
			// Beta: if (var18 == 0) { var19 = var2 - 1; } (RedStoneDustTile.java:122-124)
			if (dir == 0)
				checkX = x - 1;
			// Beta: if (var18 == 1) { var19++; } (RedStoneDustTile.java:126-128)
			if (dir == 1)
				checkX++;
			// Beta: if (var18 == 2) { var20 = var4 - 1; } (RedStoneDustTile.java:130-132)
			if (dir == 2)
				checkZ = z - 1;
			// Beta: if (var18 == 3) { var20++; } (RedStoneDustTile.java:134-136)
			if (dir == 3)
				checkZ++;

			// Beta: if (var1.isSolidTile(var19, var3, var20)) { var14 += 2; } (RedStoneDustTile.java:138-140)
			if (level.isSolidTile(checkX, y, checkZ))
				checkY += 2;

			// Beta: int var15 = 0; var15 = this.checkTarget(var1, var19, var3, var20, -1) (RedStoneDustTile.java:142-143)
			int_t targetPower1 = checkTarget(level, checkX, y, checkZ, -1);
			// Beta: var9 = var1.getData(var2, var3, var4) (RedStoneDustTile.java:144)
			newPower = level.getData(x, y, z);
			// Beta: if (var9 > 0) { var9--; } (RedStoneDustTile.java:145-147)
			if (newPower > 0)
				newPower--;

			// Beta: if (var15 >= 0 && var15 != var9) (RedStoneDustTile.java:149)
			if (targetPower1 >= 0 && targetPower1 != newPower)
			{
				// Beta: this.updatePowerStrength(var1, var19, var3, var20, var2, var3, var4) (RedStoneDustTile.java:150)
				updatePowerStrength(level, checkX, y, checkZ, x, y, z);
			}

			// Beta: var15 = this.checkTarget(var1, var19, var14, var20, -1) (RedStoneDustTile.java:153)
			int_t targetPower2 = checkTarget(level, checkX, checkY, checkZ, -1);
			// Beta: var9 = var1.getData(var2, var3, var4) (RedStoneDustTile.java:154)
			newPower = level.getData(x, y, z);
			// Beta: if (var9 > 0) { var9--; } (RedStoneDustTile.java:155-157)
			if (newPower > 0)
				newPower--;

			// Beta: if (var15 >= 0 && var15 != var9) (RedStoneDustTile.java:159)
			if (targetPower2 >= 0 && targetPower2 != newPower)
			{
				// Beta: this.updatePowerStrength(var1, var19, var14, var20, var2, var3, var4) (RedStoneDustTile.java:160)
				updatePowerStrength(level, checkX, checkY, checkZ, x, y, z);
			}
		}

		// Beta: if (var8 == 0 || var9 == 0) (RedStoneDustTile.java:164)
		newPower = level.getData(x, y, z);
		if (currentData == 0 || newPower == 0)
		{
			// Beta: this.toUpdate.add(new TilePos(var2, var3, var4)) (RedStoneDustTile.java:165)
			toUpdate.insert(TilePos(x, y, z));
			// Beta: this.toUpdate.add(new TilePos(var2 - 1, var3, var4)) (RedStoneDustTile.java:166)
			toUpdate.insert(TilePos(x - 1, y, z));
			// Beta: this.toUpdate.add(new TilePos(var2 + 1, var3, var4)) (RedStoneDustTile.java:167)
			toUpdate.insert(TilePos(x + 1, y, z));
			// Beta: this.toUpdate.add(new TilePos(var2, var3 - 1, var4)) (RedStoneDustTile.java:168)
			toUpdate.insert(TilePos(x, y - 1, z));
			// Beta: this.toUpdate.add(new TilePos(var2, var3 + 1, var4)) (RedStoneDustTile.java:169)
			toUpdate.insert(TilePos(x, y + 1, z));
			// Beta: this.toUpdate.add(new TilePos(var2, var3, var4 - 1)) (RedStoneDustTile.java:170)
			toUpdate.insert(TilePos(x, y, z - 1));
			// Beta: this.toUpdate.add(new TilePos(var2, var3, var4 + 1)) (RedStoneDustTile.java:171)
			toUpdate.insert(TilePos(x, y, z + 1));
		}
	}
}

// Beta: RedStoneDustTile.checkCornerChangeAt() - checks corner changes (RedStoneDustTile.java:176-186)
void RedStoneDustTile::checkCornerChangeAt(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: if (var1.getTile(var2, var3, var4) == this.id) (RedStoneDustTile.java:177)
	if (level.getTile(x, y, z) == id)
	{
		// Beta: var1.updateNeighborsAt(var2, var3, var4, this.id) (RedStoneDustTile.java:178)
		level.updateNeighborsAt(x, y, z, id);
		// Beta: var1.updateNeighborsAt(var2 - 1, var3, var4, this.id) (RedStoneDustTile.java:179)
		level.updateNeighborsAt(x - 1, y, z, id);
		// Beta: var1.updateNeighborsAt(var2 + 1, var3, var4, this.id) (RedStoneDustTile.java:180)
		level.updateNeighborsAt(x + 1, y, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3, var4 - 1, this.id) (RedStoneDustTile.java:181)
		level.updateNeighborsAt(x, y, z - 1, id);
		// Beta: var1.updateNeighborsAt(var2, var3, var4 + 1, this.id) (RedStoneDustTile.java:182)
		level.updateNeighborsAt(x, y, z + 1, id);
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (RedStoneDustTile.java:183)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 + 1, var4, this.id) (RedStoneDustTile.java:184)
		level.updateNeighborsAt(x, y + 1, z, id);
	}
}

// Beta: RedStoneDustTile.onPlace() - updates power strength (RedStoneDustTile.java:188-223)
void RedStoneDustTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: super.onPlace(var1, var2, var3, var4) (RedStoneDustTile.java:190)
	Tile::onPlace(level, x, y, z);
	// Beta: if (!var1.isOnline) (RedStoneDustTile.java:191)
	if (!level.isOnline)
	{
		// Beta: this.updatePowerStrength(var1, var2, var3, var4) (RedStoneDustTile.java:192)
		updatePowerStrength(level, x, y, z);
		// Beta: var1.updateNeighborsAt(var2, var3 + 1, var4, this.id) (RedStoneDustTile.java:193)
		level.updateNeighborsAt(x, y + 1, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (RedStoneDustTile.java:194)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: this.checkCornerChangeAt(var1, var2 - 1, var3, var4) (RedStoneDustTile.java:195)
		checkCornerChangeAt(level, x - 1, y, z);
		// Beta: this.checkCornerChangeAt(var1, var2 + 1, var3, var4) (RedStoneDustTile.java:196)
		checkCornerChangeAt(level, x + 1, y, z);
		// Beta: this.checkCornerChangeAt(var1, var2, var3, var4 - 1) (RedStoneDustTile.java:197)
		checkCornerChangeAt(level, x, y, z - 1);
		// Beta: this.checkCornerChangeAt(var1, var2, var3, var4 + 1) (RedStoneDustTile.java:198)
		checkCornerChangeAt(level, x, y, z + 1);
		
		// Beta: if (var1.isSolidTile(var2 - 1, var3, var4)) (RedStoneDustTile.java:199)
		if (level.isSolidTile(x - 1, y, z))
		{
			// Beta: this.checkCornerChangeAt(var1, var2 - 1, var3 + 1, var4) (RedStoneDustTile.java:200)
			checkCornerChangeAt(level, x - 1, y + 1, z);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2 - 1, var3 - 1, var4) (RedStoneDustTile.java:202)
			checkCornerChangeAt(level, x - 1, y - 1, z);
		}

		// Beta: if (var1.isSolidTile(var2 + 1, var3, var4)) (RedStoneDustTile.java:205)
		if (level.isSolidTile(x + 1, y, z))
		{
			// Beta: this.checkCornerChangeAt(var1, var2 + 1, var3 + 1, var4) (RedStoneDustTile.java:206)
			checkCornerChangeAt(level, x + 1, y + 1, z);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2 + 1, var3 - 1, var4) (RedStoneDustTile.java:208)
			checkCornerChangeAt(level, x + 1, y - 1, z);
		}

		// Beta: if (var1.isSolidTile(var2, var3, var4 - 1)) (RedStoneDustTile.java:211)
		if (level.isSolidTile(x, y, z - 1))
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 + 1, var4 - 1) (RedStoneDustTile.java:212)
			checkCornerChangeAt(level, x, y + 1, z - 1);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 - 1, var4 - 1) (RedStoneDustTile.java:214)
			checkCornerChangeAt(level, x, y - 1, z - 1);
		}

		// Beta: if (var1.isSolidTile(var2, var3, var4 + 1)) (RedStoneDustTile.java:217)
		if (level.isSolidTile(x, y, z + 1))
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 + 1, var4 + 1) (RedStoneDustTile.java:218)
			checkCornerChangeAt(level, x, y + 1, z + 1);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 - 1, var4 + 1) (RedStoneDustTile.java:220)
			checkCornerChangeAt(level, x, y - 1, z + 1);
		}
	}
}

// Beta: RedStoneDustTile.onRemove() - updates power strength (RedStoneDustTile.java:225-260)
void RedStoneDustTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: super.onRemove(var1, var2, var3, var4) (RedStoneDustTile.java:227)
	Tile::onRemove(level, x, y, z);
	// Beta: if (!var1.isOnline) (RedStoneDustTile.java:228)
	if (!level.isOnline)
	{
		// Beta: var1.updateNeighborsAt(var2, var3 + 1, var4, this.id) (RedStoneDustTile.java:229)
		level.updateNeighborsAt(x, y + 1, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (RedStoneDustTile.java:230)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: this.updatePowerStrength(var1, var2, var3, var4) (RedStoneDustTile.java:231)
		updatePowerStrength(level, x, y, z);
		// Beta: this.checkCornerChangeAt(var1, var2 - 1, var3, var4) (RedStoneDustTile.java:232)
		checkCornerChangeAt(level, x - 1, y, z);
		// Beta: this.checkCornerChangeAt(var1, var2 + 1, var3, var4) (RedStoneDustTile.java:233)
		checkCornerChangeAt(level, x + 1, y, z);
		// Beta: this.checkCornerChangeAt(var1, var2, var3, var4 - 1) (RedStoneDustTile.java:234)
		checkCornerChangeAt(level, x, y, z - 1);
		// Beta: this.checkCornerChangeAt(var1, var2, var3, var4 + 1) (RedStoneDustTile.java:235)
		checkCornerChangeAt(level, x, y, z + 1);
		
		// Beta: if (var1.isSolidTile(var2 - 1, var3, var4)) (RedStoneDustTile.java:236)
		if (level.isSolidTile(x - 1, y, z))
		{
			// Beta: this.checkCornerChangeAt(var1, var2 - 1, var3 + 1, var4) (RedStoneDustTile.java:237)
			checkCornerChangeAt(level, x - 1, y + 1, z);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2 - 1, var3 - 1, var4) (RedStoneDustTile.java:239)
			checkCornerChangeAt(level, x - 1, y - 1, z);
		}

		// Beta: if (var1.isSolidTile(var2 + 1, var3, var4)) (RedStoneDustTile.java:242)
		if (level.isSolidTile(x + 1, y, z))
		{
			// Beta: this.checkCornerChangeAt(var1, var2 + 1, var3 + 1, var4) (RedStoneDustTile.java:243)
			checkCornerChangeAt(level, x + 1, y + 1, z);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2 + 1, var3 - 1, var4) (RedStoneDustTile.java:245)
			checkCornerChangeAt(level, x + 1, y - 1, z);
		}

		// Beta: if (var1.isSolidTile(var2, var3, var4 - 1)) (RedStoneDustTile.java:248)
		if (level.isSolidTile(x, y, z - 1))
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 + 1, var4 - 1) (RedStoneDustTile.java:249)
			checkCornerChangeAt(level, x, y + 1, z - 1);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 - 1, var4 - 1) (RedStoneDustTile.java:251)
			checkCornerChangeAt(level, x, y - 1, z - 1);
		}

		// Beta: if (var1.isSolidTile(var2, var3, var4 + 1)) (RedStoneDustTile.java:254)
		if (level.isSolidTile(x, y, z + 1))
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 + 1, var4 + 1) (RedStoneDustTile.java:255)
			checkCornerChangeAt(level, x, y + 1, z + 1);
		}
		else
		{
			// Beta: this.checkCornerChangeAt(var1, var2, var3 - 1, var4 + 1) (RedStoneDustTile.java:257)
			checkCornerChangeAt(level, x, y - 1, z + 1);
		}
	}
}

// Beta: RedStoneDustTile.checkTarget() - checks target power (RedStoneDustTile.java:262-269)
int_t RedStoneDustTile::checkTarget(Level &level, int_t x, int_t y, int_t z, int_t currentPower)
{
	// Beta: if (var1.getTile(var2, var3, var4) != this.id) (RedStoneDustTile.java:263)
	if (level.getTile(x, y, z) != id)
	{
		// Beta: return var5 (RedStoneDustTile.java:264)
		return currentPower;
	}
	else
	{
		// Beta: int var6 = var1.getData(var2, var3, var4) (RedStoneDustTile.java:266)
		int_t targetData = level.getData(x, y, z);
		// Beta: return var6 > var5 ? var6 : var5 (RedStoneDustTile.java:267)
		return targetData > currentPower ? targetData : currentPower;
	}
}

// Beta: RedStoneDustTile.neighborChanged() - updates power strength (RedStoneDustTile.java:271-285)
void RedStoneDustTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: if (!var1.isOnline) (RedStoneDustTile.java:273)
	if (!level.isOnline)
	{
		// Beta: int var6 = var1.getData(var2, var3, var4) (RedStoneDustTile.java:274)
		int_t data = level.getData(x, y, z);
		// Beta: boolean var7 = this.mayPlace(var1, var2, var3, var4) (RedStoneDustTile.java:275)
		bool canPlace = mayPlace(level, x, y, z);
		// Beta: if (!var7) (RedStoneDustTile.java:276)
		if (!canPlace)
		{
			// Beta: this.spawnResources(var1, var2, var3, var4, var6) (RedStoneDustTile.java:277)
			spawnResources(level, x, y, z, data);
			// Beta: var1.setTile(var2, var3, var4, 0) (RedStoneDustTile.java:278)
			level.setTile(x, y, z, 0);
		}
		else
		{
			// Beta: this.updatePowerStrength(var1, var2, var3, var4) (RedStoneDustTile.java:280)
			updatePowerStrength(level, x, y, z);
		}

		// Beta: super.neighborChanged(var1, var2, var3, var4, var5) (RedStoneDustTile.java:283)
		Tile::neighborChanged(level, x, y, z, tile);
	}
}

// Beta: RedStoneDustTile.getResource() - returns redstone item (RedStoneDustTile.java:287-290)
int_t RedStoneDustTile::getResource(int_t data, Random &random)
{
	// Beta: return Item.redStone.id (RedStoneDustTile.java:289)
	// Alpha 1.2.6: redstone item ID is 75, shiftedIndex = 256 + 75 = 331
	return Items::redstone->getShiftedIndex();
}

// Beta: RedStoneDustTile.getDirectSignal() - returns direct signal (RedStoneDustTile.java:292-295)
bool RedStoneDustTile::getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: return !this.shouldSignal ? false : this.getSignal(var1, var2, var3, var4, var5) (RedStoneDustTile.java:294)
	return !shouldSignal ? false : getSignal(level, x, y, z, facing);
}

// Beta: RedStoneDustTile.getSignal() - returns signal strength (RedStoneDustTile.java:297-342)
bool RedStoneDustTile::getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: if (!this.shouldSignal) { return false; } (RedStoneDustTile.java:299-301)
	if (!shouldSignal)
		return false;
	// Beta: else if (var1.getData(var2, var3, var4) == 0) { return false; } (RedStoneDustTile.java:301-303)
	else if (level.getData(x, y, z) == 0)
		return false;
	// Beta: else if (var5 == 1) { return true; } (RedStoneDustTile.java:303-305)
	else if (facing == 1)
		return true;
	else
	{
		// Beta: boolean var6 = shouldConnectTo(var1, var2 - 1, var3, var4) || !var1.isSolidTile(var2 - 1, var3, var4) && shouldConnectTo(var1, var2 - 1, var3 - 1, var4) (RedStoneDustTile.java:306-307)
		bool hasWest = shouldConnectTo(level, x - 1, y, z) || (!level.isSolidTile(x - 1, y, z) && shouldConnectTo(level, x - 1, y - 1, z));
		// Beta: boolean var7 = shouldConnectTo(var1, var2 + 1, var3, var4) || !var1.isSolidTile(var2 + 1, var3, var4) && shouldConnectTo(var1, var2 + 1, var3 - 1, var4) (RedStoneDustTile.java:308-309)
		bool hasEast = shouldConnectTo(level, x + 1, y, z) || (!level.isSolidTile(x + 1, y, z) && shouldConnectTo(level, x + 1, y - 1, z));
		// Beta: boolean var8 = shouldConnectTo(var1, var2, var3, var4 - 1) || !var1.isSolidTile(var2, var3, var4 - 1) && shouldConnectTo(var1, var2, var3 - 1, var4 - 1) (RedStoneDustTile.java:310-311)
		bool hasNorth = shouldConnectTo(level, x, y, z - 1) || (!level.isSolidTile(x, y, z - 1) && shouldConnectTo(level, x, y - 1, z - 1));
		// Beta: boolean var9 = shouldConnectTo(var1, var2, var3, var4 + 1) || !var1.isSolidTile(var2, var3, var4 + 1) && shouldConnectTo(var1, var2, var3 - 1, var4 + 1) (RedStoneDustTile.java:312-313)
		bool hasSouth = shouldConnectTo(level, x, y, z + 1) || (!level.isSolidTile(x, y, z + 1) && shouldConnectTo(level, x, y - 1, z + 1));
		
		// Beta: if (!var1.isSolidTile(var2, var3 + 1, var4)) (RedStoneDustTile.java:314)
		if (!level.isSolidTile(x, y + 1, z))
		{
			// Beta: if (var1.isSolidTile(var2 - 1, var3, var4) && shouldConnectTo(var1, var2 - 1, var3 + 1, var4)) { var6 = true; } (RedStoneDustTile.java:315-317)
			if (level.isSolidTile(x - 1, y, z) && shouldConnectTo(level, x - 1, y + 1, z))
				hasWest = true;
			// Beta: if (var1.isSolidTile(var2 + 1, var3, var4) && shouldConnectTo(var1, var2 + 1, var3 + 1, var4)) { var7 = true; } (RedStoneDustTile.java:319-321)
			if (level.isSolidTile(x + 1, y, z) && shouldConnectTo(level, x + 1, y + 1, z))
				hasEast = true;
			// Beta: if (var1.isSolidTile(var2, var3, var4 - 1) && shouldConnectTo(var1, var2, var3 + 1, var4 - 1)) { var8 = true; } (RedStoneDustTile.java:323-325)
			if (level.isSolidTile(x, y, z - 1) && shouldConnectTo(level, x, y + 1, z - 1))
				hasNorth = true;
			// Beta: if (var1.isSolidTile(var2, var3, var4 + 1) && shouldConnectTo(var1, var2, var3 + 1, var4 + 1)) { var9 = true; } (RedStoneDustTile.java:327-329)
			if (level.isSolidTile(x, y, z + 1) && shouldConnectTo(level, x, y + 1, z + 1))
				hasSouth = true;
		}

		// Beta: if (!var8 && !var7 && !var6 && !var9 && var5 >= 2 && var5 <= 5) { return true; } (RedStoneDustTile.java:332-334)
		if (!hasNorth && !hasEast && !hasWest && !hasSouth && facing >= 2 && facing <= 5)
			return true;
		// Beta: else if (var5 == 2 && var8 && !var6 && !var7) { return true; } (RedStoneDustTile.java:334-336)
		else if (facing == 2 && hasNorth && !hasWest && !hasEast)
			return true;
		// Beta: else if (var5 == 3 && var9 && !var6 && !var7) { return true; } (RedStoneDustTile.java:336-338)
		else if (facing == 3 && hasSouth && !hasWest && !hasEast)
			return true;
		else
		{
			// Beta: return var5 == 4 && var6 && !var8 && !var9 ? true : var5 == 5 && var7 && !var8 && !var9 (RedStoneDustTile.java:339)
			return (facing == 4 && hasWest && !hasNorth && !hasSouth) ? true : (facing == 5 && hasEast && !hasNorth && !hasSouth);
		}
	}
}

// Beta: RedStoneDustTile.isSignalSource() - returns shouldSignal (RedStoneDustTile.java:344-347)
bool RedStoneDustTile::isSignalSource()
{
	// Beta: return this.shouldSignal (RedStoneDustTile.java:346)
	return shouldSignal;
}

// Beta: RedStoneDustTile.animateTick() - adds reddust particles (RedStoneDustTile.java:349-357)
void RedStoneDustTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: if (var1.getData(var2, var3, var4) > 0) (RedStoneDustTile.java:351)
	if (level.getData(x, y, z) > 0)
	{
		// Beta: double var6 = var2 + 0.5 + (var5.nextFloat() - 0.5) * 0.2 (RedStoneDustTile.java:352)
		double px = (double)x + 0.5 + (random.nextFloat() - 0.5) * 0.2;
		// Beta: double var8 = var3 + 0.0625F (RedStoneDustTile.java:353)
		double py = (double)y + 0.0625F;
		// Beta: double var10 = var4 + 0.5 + (var5.nextFloat() - 0.5) * 0.2 (RedStoneDustTile.java:354)
		double pz = (double)z + 0.5 + (random.nextFloat() - 0.5) * 0.2;
		// Beta: var1.addParticle("reddust", var6, var8, var10, 0.0, 0.0, 0.0) (RedStoneDustTile.java:355)
		level.addParticle(u"reddust", px, py, pz, 0.0, 0.0, 0.0);
	}
}

// Beta: RedStoneDustTile.shouldConnectTo() - static helper (RedStoneDustTile.java:359-366)
bool RedStoneDustTile::shouldConnectTo(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: int var4 = var0.getTile(var1, var2, var3) (RedStoneDustTile.java:360)
	int_t tileId = level.getTile(x, y, z);
	// Beta: if (var4 == Tile.redStoneDust.id) { return true; } (RedStoneDustTile.java:361-363)
	// Use ID 55 directly to avoid circular dependency
	if (tileId == 55)
		return true;
	else
	{
		// Beta: return var4 == 0 ? false : Tile.tiles[var4].isSignalSource() (RedStoneDustTile.java:364)
		return tileId == 0 ? false : Tile::tiles[tileId]->isSignalSource();
	}
}
