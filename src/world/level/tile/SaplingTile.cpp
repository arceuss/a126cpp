#include "world/level/tile/SaplingTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/FarmTile.h"
#include "java/Random.h"

// Beta: Sapling.java - extends Bush
SaplingTile::SaplingTile(int_t id, int_t tex) : FlowerTile(id, tex)
{
	// Beta: Sapling constructor sets shape (Sapling.java:10-14)
	// float var3 = 0.4F;
	// this.setShape(0.5F - var3, 0.0F, 0.5F - var3, 0.5F + var3, var3 * 2.0F, 0.5F + var3);
	// = (0.1, 0.0, 0.1, 0.9, 0.8, 0.9)
	float size = 0.4f;  // Beta: var3 = 0.4F
	setShape(0.5f - size, 0.0f, 0.5f - size, 0.5f + size, size * 2.0f, 0.5f + size);
}

bool SaplingTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Bush.mayPlace() - checks if can be placed on block below (Bush.java:18-20)
	// return this.mayPlaceOn(var1.getTile(var2, var3 - 1, var4));
	return mayPlaceOn(level.getTile(x, y - 1, z));
}

bool SaplingTile::mayPlaceOn(int_t blockId)
{
	// Beta: Bush.mayPlaceOn() - checks if block below is valid (Bush.java:22-24)
	// return var1 == Tile.grass.id || var1 == Tile.dirt.id || var1 == Tile.farmland.id;
	// Use the same logic as FlowerTile::canThisPlantGrowOnThisBlockID() for consistency
	return blockId == Tile::grass.id || blockId == Tile::dirt.id || blockId == Tile::farmland.id;
}

void SaplingTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: Bush.tick() - calls checkAlive() (Bush.java:33-35)
	checkAlive(level, x, y, z);
	
	// Beta: Sapling.tick() - checks growth after survival check (Sapling.java:19-26)
	// Check if light level >= 9 and random chance (Sapling.java:19)
	if (level.getRawBrightness(x, y + 1, z) >= 9 && random.nextInt(5) == 0)
	{
		int_t data = level.getData(x, y, z);
		if (data < 15)
		{
			// Beta: Increment growth stage (Sapling.java:21-22)
			level.setData(x, y, z, data + 1);
		}
		else
		{
			// Beta: Grow tree (Sapling.java:24)
			growTree(level, x, y, z, random);
		}
	}
}

void SaplingTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: Bush.neighborChanged() - calls super then checkAlive() (Bush.java:27-30)
	Tile::neighborChanged(level, x, y, z, tile);
	checkAlive(level, x, y, z);
}

bool SaplingTile::canSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Bush.canSurvive() - checks light level and valid block below (Bush.java:45-47)
	// return (var1.getRawBrightness(var2, var3, var4) >= 8 || var1.canSeeSky(var2, var3, var4)) && this.mayPlaceOn(var1.getTile(var2, var3 - 1, var4));
	int_t lightValue = level.getRawBrightness(x, y, z);
	bool hasLight = lightValue >= 8 || level.canSeeSky(x, y, z);
	return hasLight && mayPlaceOn(level.getTile(x, y - 1, z));
}

void SaplingTile::checkAlive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Bush.checkAlive() - checks if can survive, drops if not (Bush.java:37-42)
	if (!canSurvive(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
	}
}

void SaplingTile::growTree(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: Sapling.growTree() - generates tree (Sapling.java:29-39)
	// TODO: Implement tree generation (TreeFeature/BasicTree)
	// For now, just remove the sapling (tree generation not implemented)
	level.setTileNoUpdate(x, y, z, 0);
	
	// Beta: Create TreeFeature or BasicTree (10% chance for BasicTree)
	// Object var6 = new TreeFeature();
	// if (var5.nextInt(10) == 0) {
	//     var6 = new BasicTree();
	// }
	// if (!((Feature)var6).place(var1, var5, var2, var3, var4)) {
	//     var1.setTileNoUpdate(var2, var3, var4, this.id);
	// }
	
	// Placeholder: If tree generation fails, restore sapling
	// For now, just leave it removed since tree generation isn't implemented
}
