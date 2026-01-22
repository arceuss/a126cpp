#include "world/level/tile/CropTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FarmTile.h"
#include "java/Random.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"
#include "Facing.h"

// Beta: CropTile.java - ID 59, texture 88, extends Bush
CropTile::CropTile(int_t id, int_t tex) : FlowerTile(id, tex)
{
	// Beta: CropTile constructor sets shape to 0.5 +/- 0.5, height 0.25 (CropTile.java:10-15)
	float size = 0.5f;  // Beta: var3 = 0.5F
	setShape(0.5f - size, 0.0f, 0.5f - size, 0.5f + size, 0.25f, 0.5f + size);
	// Note: FlowerTile constructor already calls setTicking(true), so crops will tick
}

bool CropTile::canThisPlantGrowOnThisBlockID(int_t blockId)
{
	// Beta: CropTile.mayPlaceOn() returns true only for farmland (CropTile.java:19-21)
	return blockId == Tile::farmland.id;
}

void CropTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: CropTile.tick() - calls super.tick() then grows crops (CropTile.java:24-35)
	FlowerTile::tick(level, x, y, z, random);
	
	// Beta: Check light level (must be >= 9) (CropTile.java:26)
	if (level.getRawBrightness(x, y + 1, z) >= 9)
	{
		int_t data = level.getData(x, y, z);
		if (data < 7)  // Beta: if (var6 < 7) (CropTile.java:28)
		{
			float growthSpeed = getGrowthSpeed(level, x, y, z);
			// Beta: random.nextInt((int)(100.0F / var7)) == 0 (CropTile.java:30)
			if (random.nextInt((int_t)(100.0f / growthSpeed)) == 0)
			{
				level.setData(x, y, z, ++data);  // Beta: var1.setData(var2, var3, var4, ++var6) (CropTile.java:31)
			}
		}
	}
}

void CropTile::growCropsToMax(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: CropTile.growCropsToMax() sets data to 7 (CropTile.java:37-39)
	level.setData(x, y, z, 7);
}

float CropTile::getGrowthSpeed(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: CropTile.getGrowthSpeed() - calculates growth speed based on farmland hydration (CropTile.java:41-79)
	float speed = 1.0f;  // Beta: var5 = 1.0F (CropTile.java:42)
	
	// Beta: Get neighboring crop blocks (CropTile.java:43-53)
	int_t tileN = level.getTile(x, y, z - 1);
	int_t tileS = level.getTile(x, y, z + 1);
	int_t tileW = level.getTile(x - 1, y, z);
	int_t tileE = level.getTile(x + 1, y, z);
	int_t tileNW = level.getTile(x - 1, y, z - 1);
	int_t tileNE = level.getTile(x + 1, y, z - 1);
	int_t tileSE = level.getTile(x + 1, y, z + 1);
	int_t tileSW = level.getTile(x - 1, y, z + 1);
	
	bool hasEWCrops = (tileW == this->id || tileE == this->id);  // Beta: var14 (CropTile.java:51)
	bool hasNSCrops = (tileN == this->id || tileS == this->id);  // Beta: var15 (CropTile.java:52)
	bool hasDiagCrops = (tileNW == this->id || tileNE == this->id || tileSE == this->id || tileSW == this->id);  // Beta: var16 (CropTile.java:53)
	
	// Beta: Check farmland hydration in 3x3 area (CropTile.java:55-72)
	for (int_t dx = x - 1; dx <= x + 1; ++dx)
	{
		for (int_t dz = z - 1; dz <= z + 1; ++dz)
		{
			int_t belowId = level.getTile(dx, y - 1, dz);
			float bonus = 0.0f;  // Beta: var20 (CropTile.java:58)
			if (belowId == Tile::farmland.id)
			{
				bonus = 1.0f;  // Beta: var20 = 1.0F (CropTile.java:60)
				if (level.getData(dx, y - 1, dz) > 0)  // Beta: if (var1.getData(var17, var3 - 1, var18) > 0) (CropTile.java:61)
				{
					bonus = 3.0f;  // Beta: var20 = 3.0F (CropTile.java:62)
				}
			}
			
			if (dx != x || dz != z)  // Beta: if (var17 != var2 || var18 != var4) (CropTile.java:66)
			{
				bonus /= 4.0f;  // Beta: var20 /= 4.0F (CropTile.java:67)
			}
			
			speed += bonus;  // Beta: var5 += var20 (CropTile.java:70)
		}
	}
	
	// Beta: Reduce speed if diagonally adjacent crops or both EW and NS crops (CropTile.java:74-76)
	if (hasDiagCrops || (hasEWCrops && hasNSCrops))
	{
		speed /= 2.0f;  // Beta: var5 /= 2.0F (CropTile.java:75)
	}
	
	return speed;
}

int_t CropTile::getTexture(Facing face, int_t data)
{
	// Beta: CropTile.getTexture() returns tex + data (CropTile.java:82-88)
	if (data < 0)
	{
		data = 7;
	}
	return this->tex + data;
}

Tile::Shape CropTile::getRenderShape()
{
	// Beta: CropTile.getRenderShape() returns 6 (SHAPE_ROWS) (CropTile.java:90-93)
	return SHAPE_ROWS;
}

void CropTile::destroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	// Beta: CropTile.destroy() - spawns seeds when broken (CropTile.java:96-111)
	FlowerTile::destroy(level, x, y, z, data);
	
	// Beta: Only spawn seeds if not online (CropTile.java:98)
	if (!level.isOnline)
	{
		// Beta: Spawn 0-3 seeds based on data (CropTile.java:99-109)
		for (int_t i = 0; i < 3; ++i)
		{
			if (level.random.nextInt(15) <= data)  // Beta: if (var1.random.nextInt(15) <= var5) (CropTile.java:100)
			{
				float spread = 0.7f;  // Beta: var7 = 0.7F (CropTile.java:101)
				float offsetX = level.random.nextFloat() * spread + (1.0f - spread) * 0.5f;  // Beta: var8 (CropTile.java:102)
				float offsetY = level.random.nextFloat() * spread + (1.0f - spread) * 0.5f;  // Beta: var9 (CropTile.java:103)
				float offsetZ = level.random.nextFloat() * spread + (1.0f - spread) * 0.5f;  // Beta: var10 (CropTile.java:104)
				
				// Beta: Create ItemEntity with seeds (CropTile.java:105-107)
				// Seeds item ID = 256 + 39 = 295
				ItemStack seedStack(295, 1);  // Beta: new ItemInstance(Item.seeds)
				double entityX = (double)x + offsetX;
				double entityY = (double)y + offsetY;
				double entityZ = (double)z + offsetZ;
				auto itemEntity = std::make_shared<EntityItem>(level, entityX, entityY, entityZ, seedStack);
				itemEntity->throwTime = 10;  // Beta: var11.throwTime = 10 (CropTile.java:106)
				level.addEntity(itemEntity);  // Beta: var1.addEntity(var11) (CropTile.java:107)
			}
		}
	}
}

int_t CropTile::getResource(int_t data, Random &random)
{
	// Beta: CropTile.getResource() returns wheat if fully grown (data == 7), otherwise -1 (CropTile.java:114-116)
	// Wheat item ID = 256 + 40 = 296
	return data == 7 ? 296 : -1;
}

int_t CropTile::getResourceCount(Random &random)
{
	// Beta: CropTile.getResourceCount() returns 1 (CropTile.java:119-121)
	return 1;
}
