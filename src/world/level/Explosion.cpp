#include "world/level/Explosion.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FireTile.h"
#include "util/Mth.h"
#include <cmath>

Explosion::Explosion(Level &level, Entity *source, double x, double y, double z, float radius)
	: level(level), source(source), x(x), y(y), z(z), r(radius), random()
{
}

void Explosion::explode()
{
	float var1 = r;
	byte_t var2 = 16;

	for (int_t var3 = 0; var3 < var2; var3++)
	{
		for (int_t var4 = 0; var4 < var2; var4++)
		{
			for (int_t var5 = 0; var5 < var2; var5++)
			{
				if (var3 == 0 || var3 == var2 - 1 || var4 == 0 || var4 == var2 - 1 || var5 == 0 || var5 == var2 - 1)
				{
					double var6 = var3 / (var2 - 1.0f) * 2.0f - 1.0f;
					double var8 = var4 / (var2 - 1.0f) * 2.0f - 1.0f;
					double var10 = var5 / (var2 - 1.0f) * 2.0f - 1.0f;
					double var12 = Mth::sqrt(var6 * var6 + var8 * var8 + var10 * var10);
					var6 /= var12;
					var8 /= var12;
					var10 /= var12;
					float var14 = r * (0.7f + level.random.nextFloat() * 0.6f);
					double var15 = x;
					double var17 = y;
					double var19 = z;

					for (float var21 = 0.3f; var14 > 0.0f; var14 -= var21 * 0.75f)
					{
						int_t var22 = Mth::floor(var15);
						int_t var23 = Mth::floor(var17);
						int_t var24 = Mth::floor(var19);
						int_t var25 = level.getTile(var22, var23, var24);
						if (var25 > 0)
						{
							var14 -= (Tile::tiles[var25]->getExplosionResistance() + 0.3f) * var21;
						}

						if (var14 > 0.0f)
						{
							toBlow.insert(TilePos(var22, var23, var24));
						}

						var15 += var6 * var21;
						var17 += var8 * var21;
						var19 += var10 * var21;
					}
				}
			}
		}
	}

	r *= 2.0f;
	int_t var29 = Mth::floor(x - r - 1.0);
	int_t var30 = Mth::floor(x + r + 1.0);
	int_t var31 = Mth::floor(y - r - 1.0);
	int_t var33 = Mth::floor(y + r + 1.0);
	int_t var7 = Mth::floor(z - r - 1.0);
	int_t var35 = Mth::floor(z + r + 1.0);
	AABB *expandedAABB = AABB::newTemp(var29, var31, var7, var30, var33, var35);
	std::vector<std::shared_ptr<Entity>> var9 = level.getEntities(source, *expandedAABB);
	Vec3 *var37 = Vec3::newTemp(x, y, z);

	for (size_t var11 = 0; var11 < var9.size(); var11++)
	{
		Entity *var39 = var9[var11].get();
		double var13 = var39->distanceTo(x, y, z) / r;
		if (var13 <= 1.0)
		{
			double var43 = var39->x - x;
			double var46 = var39->y - y;
			double var49 = var39->z - z;
			double var51 = Mth::sqrt(var43 * var43 + var46 * var46 + var49 * var49);
			var43 /= var51;
			var46 /= var51;
			var49 /= var51;
			double var52 = level.getSeenPercent(*var37, var39->bb);
			double var53 = (1.0 - var13) * var52;
			var39->hurt(source, (int_t)((var53 * var53 + var53) / 2.0 * 8.0 * r + 1.0));
			var39->xd += var43 * var53;
			var39->yd += var46 * var53;
			var39->zd += var49 * var53;
		}
	}

	r = var1;
	std::vector<TilePos> var38;
	var38.assign(toBlow.begin(), toBlow.end());
	if (fire)
	{
		for (int_t var40 = var38.size() - 1; var40 >= 0; var40--)
		{
			TilePos var41 = var38[var40];
			int_t var42 = var41.x;
			int_t var45 = var41.y;
			int_t var16 = var41.z;
			int_t var48 = level.getTile(var42, var45, var16);
			int_t var18 = level.getTile(var42, var45 - 1, var16);
			if (var48 == 0 && Tile::solid[var18] && random.nextInt(3) == 0)
			{
				level.setTile(var42, var45, var16, Tile::fire.id);
			}
		}
	}
}

void Explosion::addParticles()
{
	level.playSound(x, y, z, u"random.explode", 4.0f, (1.0f + (level.random.nextFloat() - level.random.nextFloat()) * 0.2f) * 0.7f);
	std::vector<TilePos> var1;
	var1.assign(toBlow.begin(), toBlow.end());

	for (int_t var2 = var1.size() - 1; var2 >= 0; var2--)
	{
		TilePos var3 = var1[var2];
		int_t var4 = var3.x;
		int_t var5 = var3.y;
		int_t var6 = var3.z;
		int_t var7 = level.getTile(var4, var5, var6);

		for (int_t var8 = 0; var8 < 1; var8++)
		{
			double var9 = var4 + level.random.nextFloat();
			double var11 = var5 + level.random.nextFloat();
			double var13 = var6 + level.random.nextFloat();
			double var15 = var9 - x;
			double var17 = var11 - y;
			double var19 = var13 - z;
			double var21 = Mth::sqrt(var15 * var15 + var17 * var17 + var19 * var19);
			var15 /= var21;
			var17 /= var21;
			var19 /= var21;
			double var23 = 0.5 / (var21 / r + 0.1);
			var23 *= level.random.nextFloat() * level.random.nextFloat() + 0.3f;
			var15 *= var23;
			var17 *= var23;
			var19 *= var23;
			level.addParticle(u"explode", (var9 + x * 1.0) / 2.0, (var11 + y * 1.0) / 2.0, (var13 + z * 1.0) / 2.0, var15, var17, var19);
			level.addParticle(u"smoke", var9, var11, var13, var15, var17, var19);
		}

		if (var7 > 0)
		{
			Tile::tiles[var7]->spawnResources(level, var4, var5, var6, level.getData(var4, var5, var6), 0.3f);
			level.setTile(var4, var5, var6, 0);
			// Note: wasExploded() not implemented yet - may need to add to Tile
		}
	}
}
