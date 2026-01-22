#include "world/level/levelgen/feature/OreFeature.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/StoneTile.h"
#include "java/Random.h"
#include "util/Mth.h"
#include <cmath>

// Beta 1.2: OreFeature.java
OreFeature::OreFeature(int_t tileId, int_t veinCount) : tile(tileId), count(veinCount)
{
	// Beta: OreFeature(int var1, int var2) (OreFeature.java:12-15)
	// this.tile = var1; this.count = var2;
}

bool OreFeature::place(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Beta: OreFeature.place() (OreFeature.java:18-60)
	// Beta: float var6 = var2.nextFloat() * (float) Math.PI (OreFeature.java:19)
	float var6 = random.nextFloat() * (float)Mth::PI;
	// Beta: double var7 = var3 + 8 + Mth.sin(var6) * this.count / 8.0F (OreFeature.java:20)
	double var7 = x + 8 + Mth::sin(var6) * this->count / 8.0F;
	// Beta: double var9 = var3 + 8 - Mth.sin(var6) * this.count / 8.0F (OreFeature.java:21)
	double var9 = x + 8 - Mth::sin(var6) * this->count / 8.0F;
	// Beta: double var11 = var5 + 8 + Mth.cos(var6) * this.count / 8.0F (OreFeature.java:22)
	double var11 = z + 8 + Mth::cos(var6) * this->count / 8.0F;
	// Beta: double var13 = var5 + 8 - Mth.cos(var6) * this.count / 8.0F (OreFeature.java:23)
	double var13 = z + 8 - Mth::cos(var6) * this->count / 8.0F;
	// Beta: double var15 = var4 + var2.nextInt(3) + 2 (OreFeature.java:24)
	double var15 = y + random.nextInt(3) + 2;
	// Beta: double var17 = var4 + var2.nextInt(3) + 2 (OreFeature.java:25)
	double var17 = y + random.nextInt(3) + 2;

	// Beta: for (int var19 = 0; var19 <= this.count; var19++) (OreFeature.java:27)
	for (int_t var19 = 0; var19 <= this->count; var19++)
	{
		// Beta: double var20 = var7 + (var9 - var7) * var19 / this.count (OreFeature.java:28)
		double var20 = var7 + (var9 - var7) * var19 / this->count;
		// Beta: double var22 = var15 + (var17 - var15) * var19 / this.count (OreFeature.java:29)
		double var22 = var15 + (var17 - var15) * var19 / this->count;
		// Beta: double var24 = var11 + (var13 - var11) * var19 / this.count (OreFeature.java:30)
		double var24 = var11 + (var13 - var11) * var19 / this->count;
		// Beta: double var26 = var2.nextDouble() * this.count / 16.0 (OreFeature.java:31)
		double var26 = random.nextDouble() * this->count / 16.0;
		// Beta: double var28 = (Mth.sin(var19 * (float) Math.PI / this.count) + 1.0F) * var26 + 1.0 (OreFeature.java:32)
		double var28 = (Mth::sin(var19 * (float)Mth::PI / this->count) + 1.0F) * var26 + 1.0;
		// Beta: double var30 = (Mth.sin(var19 * (float) Math.PI / this.count) + 1.0F) * var26 + 1.0 (OreFeature.java:33)
		double var30 = (Mth::sin(var19 * (float)Mth::PI / this->count) + 1.0F) * var26 + 1.0;
		// Beta: int var32 = (int)(var20 - var28 / 2.0) (OreFeature.java:34)
		int_t var32 = (int_t)(var20 - var28 / 2.0);
		// Beta: int var33 = (int)(var22 - var30 / 2.0) (OreFeature.java:35)
		int_t var33 = (int_t)(var22 - var30 / 2.0);
		// Beta: int var34 = (int)(var24 - var28 / 2.0) (OreFeature.java:36)
		int_t var34 = (int_t)(var24 - var28 / 2.0);
		// Beta: int var35 = (int)(var20 + var28 / 2.0) (OreFeature.java:37)
		int_t var35 = (int_t)(var20 + var28 / 2.0);
		// Beta: int var36 = (int)(var22 + var30 / 2.0) (OreFeature.java:38)
		int_t var36 = (int_t)(var22 + var30 / 2.0);
		// Beta: int var37 = (int)(var24 + var28 / 2.0) (OreFeature.java:39)
		int_t var37 = (int_t)(var24 + var28 / 2.0);

		// Beta: for (int var38 = var32; var38 <= var35; var38++) (OreFeature.java:41)
		for (int_t var38 = var32; var38 <= var35; var38++)
		{
			// Beta: double var39 = (var38 + 0.5 - var20) / (var28 / 2.0) (OreFeature.java:42)
			double var39 = (var38 + 0.5 - var20) / (var28 / 2.0);
			// Beta: if (var39 * var39 < 1.0) (OreFeature.java:43)
			if (var39 * var39 < 1.0)
			{
				// Beta: for (int var41 = var33; var41 <= var36; var41++) (OreFeature.java:44)
				for (int_t var41 = var33; var41 <= var36; var41++)
				{
					// Beta: double var42 = (var41 + 0.5 - var22) / (var30 / 2.0) (OreFeature.java:45)
					double var42 = (var41 + 0.5 - var22) / (var30 / 2.0);
					// Beta: if (var39 * var39 + var42 * var42 < 1.0) (OreFeature.java:46)
					if (var39 * var39 + var42 * var42 < 1.0)
					{
						// Beta: for (int var44 = var34; var44 <= var37; var44++) (OreFeature.java:47)
						for (int_t var44 = var34; var44 <= var37; var44++)
						{
							// Beta: double var45 = (var44 + 0.5 - var24) / (var28 / 2.0) (OreFeature.java:48)
							double var45 = (var44 + 0.5 - var24) / (var28 / 2.0);
							// Beta: if (var39 * var39 + var42 * var42 + var45 * var45 < 1.0 && var1.getTile(var38, var41, var44) == Tile.rock.id) (OreFeature.java:49)
							if (var39 * var39 + var42 * var42 + var45 * var45 < 1.0 && level.getTile(var38, var41, var44) == Tile::rock.id)
							{
								// Beta: var1.setTileNoUpdate(var38, var41, var44, this.tile) (OreFeature.java:50)
								level.setTileNoUpdate(var38, var41, var44, this->tile);
							}
						}
					}
				}
			}
		}
	}

	// Beta: return true (OreFeature.java:59)
	return true;
}
