#include "world/level/levelgen/feature/WorldGenLakes.h"
#include "world/level/Level.h"
#include "world/level/LightLayer.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/material/Material.h"
#include "java/Random.h"
#include <vector>
#include <cmath>

// Beta 1.2: LakeFeature.java - ported 1:1
WorldGenLakes::WorldGenLakes(int_t blockId) : liquidBlockId(blockId)
{
}

bool WorldGenLakes::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Beta: LakeFeature.java:17-123
	// Offset X and Z by 8 blocks (centered in 16x16 area)
	x -= 8;
	z -= 8;
	
	// Beta: line 21 - Find topmost non-air block by decrementing Y while block is empty
	// Beta: while (var4 > 0 && var1.isEmptyTile(var3, var4, var5))
	while (y > 0 && level.isEmptyTile(x, y, z))
		--y;
	
	// Beta: line 25 - Start 4 blocks below the topmost non-air block
	y -= 4;
	
	// Beta: line 26 - Create boolean array for lake shape (2048 = 16*16*8)
	std::vector<bool> lakeShape(2048, false);
	
	// Beta: line 27 - Generate 4-7 ellipsoids
	int_t ellipsoidCount = random.nextInt(4) + 4;  // 4-7 ellipsoids
	
	// Beta: lines 29-50 - Generate ellipsoid shapes
	for (int_t i = 0; i < ellipsoidCount; ++i)
	{
		// Beta: lines 30-35 - Random ellipsoid dimensions and centers
		double radiusX = random.nextDouble() * 6.0 + 3.0;  // 3.0-9.0
		double radiusY = random.nextDouble() * 4.0 + 2.0;  // 2.0-6.0
		double radiusZ = random.nextDouble() * 6.0 + 3.0;  // 3.0-9.0
		
		double centerX = random.nextDouble() * (16.0 - radiusX - 2.0) + 1.0 + radiusX / 2.0;
		double centerY = random.nextDouble() * (8.0 - radiusY - 4.0) + 2.0 + radiusY / 2.0;
		double centerZ = random.nextDouble() * (16.0 - radiusZ - 2.0) + 1.0 + radiusZ / 2.0;
		
		// Beta: lines 37-49 - Mark ellipsoid volume in boolean array
		for (int_t lx = 1; lx < 15; ++lx)
		{
			for (int_t lz = 1; lz < 15; ++lz)
			{
				for (int_t ly = 1; ly < 7; ++ly)
				{
					// Beta: lines 40-43 - Calculate distance from ellipsoid center
					double dx = (lx - centerX) / (radiusX / 2.0);
					double dy = (ly - centerY) / (radiusY / 2.0);
					double dz = (lz - centerZ) / (radiusZ / 2.0);
					double distSq = dx * dx + dy * dy + dz * dz;
					
					// Beta: line 44 - If inside ellipsoid, mark as lake
					if (distSq < 1.0)
					{
						int_t index = (lx * 16 + lz) * 8 + ly;
						lakeShape[index] = true;
					}
				}
			}
		}
	}
	
	// Beta: lines 52-76 - Validate lake placement
	// Check if lake would conflict with existing liquids or non-replaceable blocks
	for (int_t lx = 0; lx < 16; ++lx)
	{
		for (int_t lz = 0; lz < 16; ++lz)
		{
			for (int_t ly = 0; ly < 8; ++ly)
			{
				int_t index = (lx * 16 + lz) * 8 + ly;
				bool isLake = lakeShape[index];
				
				// Beta: lines 55-63 - Check if this position is adjacent to lake
				bool isAdjacent = !isLake && (
					(lx < 15 && lakeShape[((lx + 1) * 16 + lz) * 8 + ly]) ||
					(lx > 0 && lakeShape[((lx - 1) * 16 + lz) * 8 + ly]) ||
					(lz < 15 && lakeShape[(lx * 16 + lz + 1) * 8 + ly]) ||
					(lz > 0 && lakeShape[(lx * 16 + (lz - 1)) * 8 + ly]) ||
					(ly < 7 && lakeShape[(lx * 16 + lz) * 8 + ly + 1]) ||
					(ly > 0 && lakeShape[(lx * 16 + lz) * 8 + (ly - 1)])
				);
				
				if (isAdjacent)
				{
					const Material &mat = level.getMaterial(x + lx, y + ly, z + lz);
					
					// Beta: lines 66-68 - If Y >= 4 and material is liquid, cancel lake
					if (ly >= 4 && mat.isLiquid())
					{
						return false;
					}
					
					// Beta: line 70 - If Y < 4 and material is not solid and block is not the liquid we're placing, cancel
					// Beta: if (var10 < 4 && !var12.isSolid() && var1.getTile(var3 + var35, var4 + var10, var5 + var39) != this.tile)
					int_t adjacentTile = level.getTile(x + lx, y + ly, z + lz);
					if (ly < 4 && !mat.isSolid() && adjacentTile != liquidBlockId)
					{
						return false;
					}
				}
			}
		}
	}
	
	// Beta: lines 78-86 - Place lake blocks
	// Beta: var1.setTileNoUpdate(var3 + var36, var4 + var43, var5 + var40, var43 >= 4 ? 0 : this.tile)
	for (int_t lx = 0; lx < 16; ++lx)
	{
		for (int_t lz = 0; lz < 16; ++lz)
		{
			for (int_t ly = 0; ly < 8; ++ly)
			{
				int_t index = (lx * 16 + lz) * 8 + ly;
				if (lakeShape[index])
				{
				int_t worldX = x + lx;
				int_t worldY = y + ly;
				int_t worldZ = z + lz;
				
				// Beta: line 82 - Y >= 4 becomes air, Y < 4 becomes liquid
				if (ly >= 4)
				{
					level.setTileNoUpdate(worldX, worldY, worldZ, 0);
				}
				else
				{
					// Beta: setTileNoUpdate sets tile ID, metadata defaults to 0 (source block)
					// This allows the fluid to flow and fill the entire lake area
					level.setTileAndDataNoUpdate(worldX, worldY, worldZ, liquidBlockId, 0);
				}
				}
			}
		}
	}
	
	// Beta: lines 88-98 - Convert dirt to grass under lake surface
	for (int_t lx = 0; lx < 16; ++lx)
	{
		for (int_t lz = 0; lz < 16; ++lz)
		{
			for (int_t ly = 4; ly < 8; ++ly)
			{
				int_t index = (lx * 16 + lz) * 8 + ly;
				if (lakeShape[index])
				{
					int_t belowX = x + lx;
					int_t belowY = y + ly - 1;
					int_t belowZ = z + lz;
					
					// Beta: lines 92-94 - If block below is dirt and has sky light, convert to grass
					if (level.getTile(belowX, belowY, belowZ) == Tile::dirt.id)
					{
						// Beta: Check sky light value (getBrightness(LightLayer.Sky, ...) > 0)
						// Beta uses getBrightness(LightLayer.Sky, x, y, z) > 0
						if (level.getBrightness(LightLayer::Sky, belowX, belowY + 1, belowZ) > 0)
						{
							level.setTileNoUpdate(belowX, belowY, belowZ, Tile::grass.id);
						}
					}
				}
			}
		}
	}
	
	// Beta: lines 100-119 - For lava lakes, convert adjacent solid blocks to stone
	// Beta: if (Tile.tiles[this.tile].material == Material.lava)
	// Beta 1.2: Tile.lava = ID 10, Tile.calmLava = ID 11
	if (liquidBlockId == 10 || liquidBlockId == 11)
	{
		for (int_t lx = 0; lx < 16; ++lx)
		{
			for (int_t lz = 0; lz < 16; ++lz)
			{
				for (int_t ly = 0; ly < 8; ++ly)
				{
					int_t index = (lx * 16 + lz) * 8 + ly;
					bool isLake = lakeShape[index];
					
					// Beta: lines 104-112 - Check if adjacent to lake
					bool isAdjacent = !isLake && (
						(lx < 15 && lakeShape[((lx + 1) * 16 + lz) * 8 + ly]) ||
						(lx > 0 && lakeShape[((lx - 1) * 16 + lz) * 8 + ly]) ||
						(lz < 15 && lakeShape[(lx * 16 + lz + 1) * 8 + ly]) ||
						(lz > 0 && lakeShape[(lx * 16 + (lz - 1)) * 8 + ly]) ||
						(ly < 7 && lakeShape[(lx * 16 + lz) * 8 + ly + 1]) ||
						(ly > 0 && lakeShape[(lx * 16 + lz) * 8 + (ly - 1)])
					);
					
					// Beta: line 113 - If adjacent and (Y < 4 or random chance) and material is solid, convert to stone
					if (isAdjacent && (ly < 4 || random.nextInt(2) != 0))
					{
						const Material &mat = level.getMaterial(x + lx, y + ly, z + lz);
						if (mat.isSolid())
						{
							level.setTileNoUpdate(x + lx, y + ly, z + lz, Tile::rock.id);
						}
					}
				}
			}
		}
	}
	
	return true;
}
