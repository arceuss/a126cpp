#include "world/level/levelgen/feature/WorldGenReed.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/ReedTile.h"
#include "world/level/material/Material.h"
#include "java/Random.h"

// Alpha: WorldGenReed.java
bool WorldGenReed::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenReed.java:6-19
	// 20 attempts to place sugar cane
	for (int_t i = 0; i < 20; ++i)
	{
		int_t px = x + random.nextInt(4) - random.nextInt(4);
		int_t py = y;
		int_t pz = z + random.nextInt(4) - random.nextInt(4);
		
		// Alpha: Check if position is air and adjacent to water
		if (level.getTile(px, py, pz) == 0)
		{
			// Check if adjacent to water (BlockReed.canPlaceBlockAt checks for water at (px-1,py-1,pz), (px+1,py-1,pz), etc.)
			// TODO: Material::water doesn't exist yet - check for water block ID instead
			// bool hasWater = level.getMaterial(px - 1, py - 1, pz) == Material::water ||
			//                 level.getMaterial(px + 1, py - 1, pz) == Material::water ||
			//                 level.getMaterial(px, py - 1, pz - 1) == Material::water ||
			//                 level.getMaterial(px, py - 1, pz + 1) == Material::water;
			// Check for water blocks adjacent (below block at y-1 in any cardinal direction)
			// Alpha: BlockReed.canPlaceBlockAt() checks for water at adjacent positions below (BlockReed.java:33-35)
			bool hasWater = level.getTile(px - 1, py - 1, pz) == 8 || level.getTile(px - 1, py - 1, pz) == 9 ||
			                level.getTile(px + 1, py - 1, pz) == 8 || level.getTile(px + 1, py - 1, pz) == 9 ||
			                level.getTile(px, py - 1, pz - 1) == 8 || level.getTile(px, py - 1, pz - 1) == 9 ||
			                level.getTile(px, py - 1, pz + 1) == 8 || level.getTile(px, py - 1, pz + 1) == 9;
			
			if (hasWater)
			{
				// Alpha: Height is 2 + random(1-3) = 2-4 blocks tall (WorldGenReed.java:10-18)
				int_t height = 2 + random.nextInt(random.nextInt(3) + 1);
				
				for (int_t h = 0; h < height; ++h)
				{
					// Check if position is valid (air above, valid block below)
					int_t reedId = Tile::getReedId();
					int_t belowId = (h == 0) ? level.getTile(px, py - 1, pz) : reedId;
					if (py + h < Level::DEPTH && level.getTile(px, py + h, pz) == 0 &&
					    (belowId == Tile::grass.id || belowId == Tile::dirt.id || belowId == reedId))
					{
						level.setTile(px, py + h, pz, reedId);  // setTile(x, y, z, tile)
					}
					else
					{
						break;  // Stop growing if blocked
					}
				}
			}
		}
	}
	return true;
}
