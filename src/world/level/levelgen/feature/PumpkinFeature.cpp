#include "world/level/levelgen/feature/PumpkinFeature.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/PumpkinTile.h"
#include "world/level/tile/GrassTile.h"
#include "java/Random.h"

// Beta 1.2: PumpkinFeature.java
bool PumpkinFeature::place(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Beta: PumpkinFeature.place() (PumpkinFeature.java:9-20)
	// Beta: for (int var6 = 0; var6 < 64; var6++) (PumpkinFeature.java:10)
	for (int_t var6 = 0; var6 < 64; var6++)
	{
		// Beta: int var7 = var3 + var2.nextInt(8) - var2.nextInt(8) (PumpkinFeature.java:11)
		int_t var7 = x + random.nextInt(8) - random.nextInt(8);
		// Beta: int var8 = var4 + var2.nextInt(4) - var2.nextInt(4) (PumpkinFeature.java:12)
		int_t var8 = y + random.nextInt(4) - random.nextInt(4);
		// Beta: int var9 = var5 + var2.nextInt(8) - var2.nextInt(8) (PumpkinFeature.java:13)
		int_t var9 = z + random.nextInt(8) - random.nextInt(8);
		// Beta: if (var1.isEmptyTile(var7, var8, var9) && var1.getTile(var7, var8 - 1, var9) == Tile.grass.id && Tile.pumpkin.mayPlace(var1, var7, var8, var9)) (PumpkinFeature.java:14)
		if (level.isEmptyTile(var7, var8, var9) && level.getTile(var7, var8 - 1, var9) == Tile::grass.id && Tile::pumpkin.mayPlace(level, var7, var8, var9))
		{
			// Beta: var1.setTileAndDataNoUpdate(var7, var8, var9, Tile.pumpkin.id, var2.nextInt(4)) (PumpkinFeature.java:15)
			level.setTileAndDataNoUpdate(var7, var8, var9, Tile::pumpkin.id, random.nextInt(4));
		}
	}

	// Beta: return true (PumpkinFeature.java:19)
	return true;
}
