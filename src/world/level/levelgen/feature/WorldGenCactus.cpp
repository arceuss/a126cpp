#include "world/level/levelgen/feature/WorldGenCactus.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/CactusTile.h"
#include "java/Random.h"

// Alpha: WorldGenCactus.java
bool WorldGenCactus::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Alpha: WorldGenCactus.java:6-19
	// 10 attempts to place cactus
	for (int_t i = 0; i < 10; ++i)
	{
		int_t px = x + random.nextInt(8) - random.nextInt(8);
		int_t py = y + random.nextInt(4) - random.nextInt(4);
		int_t pz = z + random.nextInt(8) - random.nextInt(8);
		
		if (level.getTile(px, py, pz) == 0)
		{
			// Alpha: Height is 1 + random(1-3) = 1-3 blocks tall
			int_t height = 1 + random.nextInt(random.nextInt(3) + 1);
			
			for (int_t h = 0; h < height; ++h)
			{
				// Alpha: BlockCactus.canBlockStay() - requires checking adjacent blocks are not solid and sand/cactus below (BlockCactus.java:68-80)
				int_t cactusId = Tile::getCactusId();
				int_t belowId = (h == 0) ? level.getTile(px, py - 1, pz) : cactusId;
				if (py + h < Level::DEPTH && level.getTile(px, py + h, pz) == 0 &&
				    (belowId == Tile::sand.id || belowId == cactusId))
				{
					// Check that adjacent blocks are not solid (BlockCactus requirement)
					if (!Tile::solid[level.getTile(px - 1, py + h, pz)] &&
					    !Tile::solid[level.getTile(px + 1, py + h, pz)] &&
					    !Tile::solid[level.getTile(px, py + h, pz - 1)] &&
					    !Tile::solid[level.getTile(px, py + h, pz + 1)])
					{
						level.setTile(px, py + h, pz, cactusId);  // setTile(x, y, z, tile)
					}
					else
					{
						break;  // Stop growing if blocked
					}
				}
				else
				{
					break;  // Stop growing if blocked
				}
			}
		}
	}
	return true;
}
