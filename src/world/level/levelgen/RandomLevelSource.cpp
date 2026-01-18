#include "world/level/levelgen/RandomLevelSource.h"

#include "world/level/Level.h"

#include "world/level/tile/Tile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/GravelTile.h"
#include "world/level/tile/FlowerTile.h"
#include "world/level/tile/MushroomTile.h"
#include "world/level/tile/ReedTile.h"
#include "world/level/tile/CactusTile.h"
#include "world/level/tile/IceTile.h"
#include "world/level/tile/OreTile.h"
#include "world/level/tile/RedStoneOreTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/ClayTile.h"
#include "world/level/tile/SnowTile.h"

#include "world/level/biome/BiomeSource.h"

#include "world/level/levelgen/feature/TreeFeature.h"
#include "world/level/levelgen/LargeCaveFeature.h"
#include "world/level/levelgen/feature/WorldGenFlowers.h"
#include "world/level/levelgen/feature/WorldGenReed.h"
#include "world/level/levelgen/feature/WorldGenCactus.h"
#include "world/level/levelgen/feature/WorldGenMinable.h"
#include "world/level/levelgen/feature/WorldGenLakes.h"
#include "world/level/levelgen/feature/OreFeature.h"
#include "world/level/levelgen/feature/WorldGenLiquids.h"
#include "world/level/levelgen/feature/MonsterRoomFeature.h"
#include "world/level/levelgen/feature/WorldGenBigTree.h"


TreeFeature tree_feature_lol;
LargeCaveFeature large_cave_feature;

RandomLevelSource::RandomLevelSource(Level &level, long_t seed) : level(level),
	random(seed),
	lperlinNoise1(random, 16),
	lperlinNoise2(random, 16),
	perlinNoise1(random, 8),
	perlinNoise2(random, 4),
	perlinNoise3(random, 4),
	scaleNoise(random, 10),
	depthNoise(random, 16),
	forestNoise(random, 8)
{

}

void RandomLevelSource::prepareHeights(int_t x, int_t z, ubyte_t *tiles, double *temperatures)
{
	getHeights(buffer.data(), x * BUFFER_WIDTH, 0, z * BUFFER_WIDTH, BUFFER_WIDTH_1, BUFFER_HEIGHT_1, BUFFER_WIDTH_1);

	for (int_t xi = 0; xi < BUFFER_WIDTH; xi++)
	{
		for (int_t zi = 0; zi < BUFFER_WIDTH; zi++)
		{
			for (int_t yi = 0; yi < BUFFER_HEIGHT; yi++)
			{
				double ddiv = 0.125;

				double v000 = buffer[((xi + 0) * BUFFER_WIDTH_1 + zi + 0) * BUFFER_HEIGHT_1 + yi + 0];
				double v010 = buffer[((xi + 0) * BUFFER_WIDTH_1 + zi + 1) * BUFFER_HEIGHT_1 + yi + 0];
				double v100 = buffer[((xi + 1) * BUFFER_WIDTH_1 + zi + 0) * BUFFER_HEIGHT_1 + yi + 0];
				double v110 = buffer[((xi + 1) * BUFFER_WIDTH_1 + zi + 1) * BUFFER_HEIGHT_1 + yi + 0];
				double d001 = (buffer[((xi + 0) * BUFFER_WIDTH_1 + zi + 0) * BUFFER_HEIGHT_1 + yi + 1] - v000) * ddiv;
				double d011 = (buffer[((xi + 0) * BUFFER_WIDTH_1 + zi + 1) * BUFFER_HEIGHT_1 + yi + 1] - v010) * ddiv;
				double d101 = (buffer[((xi + 1) * BUFFER_WIDTH_1 + zi + 0) * BUFFER_HEIGHT_1 + yi + 1] - v100) * ddiv;
				double d111 = (buffer[((xi + 1) * BUFFER_WIDTH_1 + zi + 1) * BUFFER_HEIGHT_1 + yi + 1] - v110) * ddiv;

				for (int_t cyi = 0; cyi < CHUNK_HEIGHT; cyi++)
				{
					double ddiv2 = 0.25;

					double vx00 = v000;
					double vx10 = v010;
					double dx00 = (v100 - v000) * ddiv2;
					double dx10 = (v110 - v010) * ddiv2;

					for (int_t cxi = 0; cxi < CHUNK_WIDTH; cxi++)
					{
						int_t i = ((cxi + CHUNK_WIDTH * xi) * (16 * 128)) | ((zi * CHUNK_WIDTH) * (128)) | (cyi + CHUNK_HEIGHT * yi);
						int_t pitch = 128;

						double ddiv3 = 0.25;

						double vxx0 = vx00;
						double dxx0 = (vx10 - vx00) * ddiv3;

						for (int_t czi = 0; czi < CHUNK_WIDTH; czi++)
						{
							double temperature = temperatures[(cxi + CHUNK_WIDTH * xi) * 16 + (czi + zi * CHUNK_WIDTH)];

							int_t tile = 0;
							
							// Alpha: Place water at Y < 64 (SEA_LEVEL + 1) (ChunkProviderGenerate.java:81-87)
							// Alpha: generateTerrain() places water/ice during terrain generation
							int_t yCoord = yi * CHUNK_HEIGHT + cyi;
							if (yCoord < Level::SEA_LEVEL + 1)
							{
								// Alpha: Ice generation logic (ChunkProviderGenerate.java:81-87)
								// Condition: var53 < 0.5D && var13 * 8 + var32 >= var7 - 1
								// var53 = temperature, var7 = 64 (SEA_LEVEL + 1)
								// So: temperature < 0.5 AND Y >= 63 (SEA_LEVEL)
								// Alpha uses raw temperature for this check, but we use biome classification for accuracy
								int_t tempIndex = (cxi + CHUNK_WIDTH * xi) * 16 + (czi + zi * CHUNK_WIDTH);
								double downfall = level.getBiomeSource().downfalls[tempIndex];
								BiomeType biome = BiomeSource::getBiome(static_cast<float>(temperature), static_cast<float>(downfall));
								
								// Alpha: Only freeze at surface (Y == 63, SEA_LEVEL) in cold biomes
								if (BiomeSource::isCold(biome) && yCoord >= Level::SEA_LEVEL)
								{
									tile = Tile::ice.id;  // Alpha: Block.blockIce.blockID (ID 79)
								}
								else
								{
									tile = Tile::calmWater.id;  // Alpha: Block.waterMoving.blockID (ID 9)
								}
							}

							if (vxx0 > 0.0)
								tile = Tile::rock.id;

							tiles[i] = tile;

							i += pitch;
							vxx0 += dxx0;
						}

						vx00 += dx00;
						vx10 += dx10;
					}

					v000 += d001;
					v010 += d011;
					v100 += d101;
					v110 += d111;
				}
			}
		}
	}
}

void RandomLevelSource::buildSurfaces(int_t x, int_t z, ubyte_t *tiles)
{
	int_t seaLevel = Level::SEA_LEVEL + 1;

	double scale = 1.0 / 32.0;
	perlinNoise2.getRegion(sandBuffer.data(), x * 16, z * 16, 0.0, 16, 16, 1, scale, scale, 1.0);
	perlinNoise2.getRegion(gravelBuffer.data(), z * 16, 109.0134, x * 16, 16, 1, 16, scale, 1.0, scale);
	perlinNoise3.getRegion(depthBuffer.data(), x * 16, z * 16, 0.0, 16, 16, 1, scale * 2.0, scale * 2.0, scale * 2.0);

	for (int_t x = 0; x < 16; x++)
	{
		for (int_t z = 0; z < 16; z++)
		{
			bool isSand = (sandBuffer[x + z * 16] + random.nextDouble() * 0.2) > 0.0;
			bool isGravel = (gravelBuffer[x + z * 16] + random.nextDouble() * 0.2) > 3.0;

			int_t depth = static_cast<int_t>(depthBuffer[x + z * 16] / 3.0 + 3.0 + random.nextDouble() * 0.25);
			int_t depthI = 0;

			int_t topTile = Tile::grass.id;
			int_t fillerTile = Tile::dirt.id;

			for (int_t y = Level::DEPTH - 1; y >= 0; y--)
			{
				int_t i = (x * 16 + z) * Level::DEPTH + y;
				
				// Bedrock
				if (y <= 0 + random.nextInt(5))
				{
					continue;
				}

				// Perform filling
				int_t oldTile = tiles[i];
				if (oldTile == 0)
				{
					depthI = -1;
					continue;
				}

				if (oldTile == Tile::rock.id)
				{
					if (depthI == -1)
					{
						if (depth <= 0)
						{
							topTile = 0;
							fillerTile = Tile::rock.id;
						}
						else if (y >= seaLevel - 4 && y <= seaLevel + 1)
						{
							topTile = Tile::grass.id;
							fillerTile = Tile::dirt.id;

							if (isGravel) topTile = 0;
							if (isGravel) fillerTile = Tile::gravel.id;
							if (isSand) topTile = Tile::sand.id;
							if (isSand) fillerTile = Tile::sand.id;
						}

						depthI = depth;
						if (y >= seaLevel - 1)
							tiles[i] = topTile;
						else
							tiles[i] = fillerTile;
						continue;
					}
					else if (depthI > 0)
					{
						depthI--;
						tiles[i] = fillerTile;
					}
				}
			}
		}
	}
}

std::shared_ptr<LevelChunk> RandomLevelSource::getChunk(int_t x, int_t z)
{
	random.setSeed(x * 341873128712LL + z * 132897987541LL);

	std::shared_ptr<LevelChunk> chunk = Util::make_shared<LevelChunk>(level, x, z);

	level.getBiomeSource().getBiomeBlock(x * 16, z * 16, 16, 16);

	double *temperatures = level.getBiomeSource().temperatures.data();
	prepareHeights(x, z, chunk->blocks.data(), temperatures);

	buildSurfaces(x, z, chunk->blocks.data());

	large_cave_feature.apply(*this, level, x, z, chunk->blocks);

	chunk->recalcHeightmap();

	return chunk;
}

void RandomLevelSource::getHeights(double *out, int_t x, int_t y, int_t z, int_t xd, int_t yd, int_t zd)
{
	double xscale = 684.412;
	double yscale = 684.412;

	double *temperatures = level.getBiomeSource().temperatures.data();
	double *downfalls = level.getBiomeSource().downfalls.data();

	scaleNoise.getRegion(sr.data(), x, z, xd, zd, 1.121, 1.121, 0.5);
	depthNoise.getRegion(dr.data(), x, z, xd, zd, 200.0, 200.0, 0.5);
	perlinNoise1.getRegion(pnr.data(), x, y, z, xd, yd, zd, xscale / 80.0, yscale / 160.0, xscale / 80.0);
	lperlinNoise1.getRegion(ar.data(), x, y, z, xd, yd, zd, xscale, yscale, xscale);
	lperlinNoise2.getRegion(br.data(), x, y, z, xd, yd, zd, xscale, yscale, xscale);

	int_t i3 = 0;
	int_t i2 = 0;

	int_t inc = 16 / xd;
	for (int_t xi = 0; xi < xd; xi++)
	{
		int_t bx = xi * inc + inc / 2;
		for (int_t zi = 0; zi < zd; zi++)
		{
			int_t bz = zi * inc + inc / 2;

			double temperature = temperatures[bx * 16 + bz];
			double downfall = downfalls[bx * 16 + bz] * temperature;

			double factor = 1.0 - downfall;
			factor *= factor;
			factor *= factor;
			factor = 1.0 - factor;

			double sv = (sr[i2] + 256.0) / 512.0;
			sv *= factor;
			if (sv > 1.0) sv = 1.0;

			double dv = dr[i2] / 8000.0;
			if (dv < 0.0) dv = -dv * 0.3;
			dv = dv * 3.0 - 2.0;

			if (dv < 0.0)
			{
				dv /= 2.0;
				if (dv < -1.0) dv = -1.0;
				dv /= 1.4;
				dv /= 2.0;
				sv = 0.0;
			}
			else
			{
				if (dv > 1.0) dv = 1.0;
				dv /= 8.0;
			}

			if (sv < 0.0)
				sv = 0.0;
			sv += 0.5;

			dv = dv * yd / 16.0;

			double height = yd / 2.0 + dv * 4.0;

			i2++;

			for (int_t yi = 0; yi < yd; yi++)
			{
				double final = 0.0;
				
				double heightFactor = (yi - height) * 12.0 / sv;
				if (heightFactor < 0.0) heightFactor *= 4.0;

				double av = ar[i3] / 512.0;
				double bv = br[i3] / 512.0;
				double pnv = (pnr[i3] / 10.0 + 1.0) / 2.0;

				if (pnv < 0.0)
					final = av;
				else if (pnv > 1.0)
					final = bv;
				else
					final = av + (bv - av) * pnv;

				final -= heightFactor;

				if (yi > yd - 4)
				{
					double factor = (yi - (yd - 4)) / 3.0f;
					final = final * (1.0 - factor) + -10.0 * factor;
				}

				out[i3] = final;
				i3++;
			}
		}
	}
}

bool RandomLevelSource::hasChunk(int_t x, int_t z)
{
	return true;
}

void RandomLevelSource::postProcess(ChunkSource &parent, int_t x, int_t z)
{
	// Alpha: ChunkProviderGenerate.populate() order (ChunkProviderGenerate.java:302-532)
	int_t cx = x * 16;
	int_t cz = z * 16;

	// Alpha: Set RNG seed for deterministic decoration (ChunkProviderGenerate.java:307-310)
	random.setSeed(level.seed);
	long_t cs0 = random.nextLong() / 2LL * 2LL + 1LL;
	long_t cs1 = random.nextLong() / 2LL * 2LL + 1LL;
	random.setSeed((x * cs0 + z * cs1) ^ level.seed);
	
	// Disable neighbor updates during world generation to prevent stack overflow
	// Fluid blocks trigger neighbor updates which can cascade infinitely during generation
	bool oldNoNeighborUpdate = level.noNeighborUpdate;
	level.noNeighborUpdate = true;

	// Lakes (water) - 1/4 chance (ChunkProviderGenerate.java:315-320)
	if (random.nextInt(4) == 0)
	{
		int_t lakeX = cx + random.nextInt(16) + 8;
		int_t lakeY = random.nextInt(128);
		int_t lakeZ = cz + random.nextInt(16) + 8;
		WorldGenLakes waterLake(8);  // Alpha: Block.waterMoving.blockID (ID 8 - flowing water)
		waterLake.generate(level, random, lakeX, lakeY, lakeZ);
	}
	
	// Lava lakes - 1/8 chance (ChunkProviderGenerate.java:322-329)
	if (random.nextInt(8) == 0)
	{
		int_t lakeX = cx + random.nextInt(16) + 8;
		int_t lakeY = random.nextInt(random.nextInt(120) + 8);  // Alpha: nested random.nextInt
		int_t lakeZ = cz + random.nextInt(16) + 8;
		// Alpha: Only place if Y < 64 or 1/10 chance (line 326)
		if (lakeY < 64 || random.nextInt(10) == 0)
		{
			WorldGenLakes lavaLake(10);  // Alpha: Block.lavaMoving.blockID (ID 10 - flowing lava)
			lavaLake.generate(level, random, lakeX, lakeY, lakeZ);
		}
	}
	
	// Dungeons (MonsterRoomFeature) - 8 attempts (Beta RandomLevelSource.java:421-425)
	// Beta: MonsterRoomFeature.place() - 8 attempts per chunk
	for (int_t i = 0; i < 8; ++i)
	{
		int_t dungeonX = cx + random.nextInt(16) + 8;
		int_t dungeonY = random.nextInt(128);
		int_t dungeonZ = cz + random.nextInt(16) + 8;
		MonsterRoomFeature dungeon;
		dungeon.place(level, random, dungeonX, dungeonY, dungeonZ);
	}

	// Clay - 10 attempts, vein size 32 (ChunkProviderGenerate.java:339-344)
	for (int_t i = 0; i < 10; ++i)
	{
		WorldGenMinable clayVein(82, 32);  // Clay block ID 82
		clayVein.generate(level, random, cx + random.nextInt(16), random.nextInt(128), cz + random.nextInt(16));
	}

	// Dirt veins - 20 attempts, vein size 32 (ChunkProviderGenerate.java:346-351)
	for (int_t i = 0; i < 20; ++i)
	{
		WorldGenMinable dirtVein(Tile::dirt.id, 32);
		dirtVein.generate(level, random, cx + random.nextInt(16), random.nextInt(128), cz + random.nextInt(16));
	}

	// Gravel veins - 10 attempts, vein size 32 (ChunkProviderGenerate.java:353-358)
	for (int_t i = 0; i < 10; ++i)
	{
		WorldGenMinable gravelVein(Tile::gravel.id, 32);
		gravelVein.generate(level, random, cx + random.nextInt(16), random.nextInt(128), cz + random.nextInt(16));
	}

	// Coal ore - 20 attempts, vein size 16, Y 0-127 (ChunkProviderGenerate.java:360-365)
	for (int_t i = 0; i < 20; ++i)
	{
		WorldGenMinable coalVein(Tile::getOreCoalId(), 16);
		#ifdef ENABLE_DECORATION_DEBUG
		if (coalVein.generate(level, random, cx + random.nextInt(16), random.nextInt(128), cz + random.nextInt(16)))
			oreBlocksPlaced += 16;
		#else
		coalVein.generate(level, random, cx + random.nextInt(16), random.nextInt(128), cz + random.nextInt(16));
		#endif
	}

	// Iron ore - 20 attempts, vein size 8, Y 0-63 (ChunkProviderGenerate.java:367-372)
	for (int_t i = 0; i < 20; ++i)
	{
		WorldGenMinable ironVein(Tile::getOreIronId(), 8);
		#ifdef ENABLE_DECORATION_DEBUG
		if (ironVein.generate(level, random, cx + random.nextInt(16), random.nextInt(64), cz + random.nextInt(16)))
			oreBlocksPlaced += 8;
		#else
		ironVein.generate(level, random, cx + random.nextInt(16), random.nextInt(64), cz + random.nextInt(16));
		#endif
	}

	// Gold ore - 2 attempts, vein size 8, Y 0-31 (ChunkProviderGenerate.java:374-379)
	for (int_t i = 0; i < 2; ++i)
	{
		WorldGenMinable goldVein(Tile::getOreGoldId(), 8);
		#ifdef ENABLE_DECORATION_DEBUG
		if (goldVein.generate(level, random, cx + random.nextInt(16), random.nextInt(32), cz + random.nextInt(16)))
			oreBlocksPlaced += 8;
		#else
		goldVein.generate(level, random, cx + random.nextInt(16), random.nextInt(32), cz + random.nextInt(16));
		#endif
	}

	// Beta: Redstone ore - 8 attempts, vein size 7, Y 0-15 (RandomLevelSource.java:470-475)
	for (int_t var32 = 0; var32 < 8; var32++)
	{
		int_t var44 = cx + random.nextInt(16);
		int_t var56 = random.nextInt(16);
		int_t var73 = cz + random.nextInt(16);
		// Beta: new OreFeature(Tile.redStoneOre.id, 7).place(this.level, this.random, var44, var56, var73) (RandomLevelSource.java:474)
		OreFeature redstoneOre(Tile::redStoneOre.id, 7);
		redstoneOre.place(level, random, var44, var56, var73);
	}

	// Beta: Diamond ore (called emeraldOre internally) - 1 attempt, vein size 7, Y 0-15 (RandomLevelSource.java:477-482)
	// Note: Beta 1.2 uses Tile.emeraldOre (ID 56) which is actually Diamond ore (descriptionId "oreDiamond")
	for (int_t var33 = 0; var33 < 1; var33++)
	{
		int_t var45 = cx + random.nextInt(16);
		int_t var57 = random.nextInt(16);
		int_t var74 = cz + random.nextInt(16);
		// Beta: new OreFeature(Tile.emeraldOre.id, 7).place(this.level, this.random, var45, var57, var74) (RandomLevelSource.java:481)
		// Tile.emeraldOre is ID 56, which is Diamond ore (oreDiamond)
		OreFeature(Tile::oreDiamond.id, 7).place(level, random, var45, var57, var74);
	}

	// Trees - Alpha order with biome adjustments (ChunkProviderGenerate.java:395-446)
	double scale = 0.5;
	int_t treeCount = (int_t)((forestNoise.getValue(cx * scale, cz * scale) / 8.0 + random.nextDouble() * 4.0 + 4.0) / 3.0);
	
	// Alpha: Base tree count adjustments
	if (random.nextInt(10) == 0)
		treeCount++;
	
	// TODO: Biome-based tree count adjustments (forest, rainforest, seasonalForest, taiga, desert, tundra, plains)
	// For now, use base count
	// treeCount += 5; // Default forest bonus
	
	// Alpha: 10% chance for big tree (ChunkProviderGenerate.java:431-433)
	WorldGenBigTree bigTree;
	TreeFeature normalTree;
	
	for (int_t i = 0; i < treeCount; ++i)
	{
		int_t tx = cx + random.nextInt(16) + 8;
		int_t tz = cz + random.nextInt(16) + 8;
		// Alpha: Use getHeightValue (ChunkProviderGenerate.java:445)
		int_t ty = level.getHeightmap(tx, tz);
		
		// Alpha: 10% chance for big tree (ChunkProviderGenerate.java:431-433)
		if (random.nextInt(10) == 0)
		{
			bigTree.init(1.0, 1.0, 1.0);
			bigTree.place(level, random, tx, ty, tz);
		}
		else
		{
			normalTree.place(level, random, tx, ty, tz);
		}
	}

	// Flowers (yellow) - 2 attempts (ChunkProviderGenerate.java:449-454)
	#ifdef ENABLE_DECORATION_DEBUG
	int_t flowersYellowPlaced = 0;
	int_t flowersRedPlaced = 0;
	#endif
	for (int_t i = 0; i < 2; ++i)
	{
		WorldGenFlowers yellowFlowers(Tile::getPlantYellowId());
		#ifdef ENABLE_DECORATION_DEBUG
		if (yellowFlowers.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8))
			flowersYellowPlaced++;
		#else
		yellowFlowers.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8);
		#endif
	}

	// Flowers (red) - 1/2 chance (ChunkProviderGenerate.java:456-461)
	if (random.nextInt(2) == 0)
	{
		WorldGenFlowers redFlowers(Tile::getPlantRedId());
		#ifdef ENABLE_DECORATION_DEBUG
		if (redFlowers.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8))
			flowersRedPlaced++;
		#else
		redFlowers.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8);
		#endif
	}

	// Mushrooms (brown) - 1/4 chance (ChunkProviderGenerate.java:463-468)
	#ifdef ENABLE_DECORATION_DEBUG
	int_t mushroomsBrownPlaced = 0;
	int_t mushroomsRedPlaced = 0;
	#endif
	if (random.nextInt(4) == 0)
	{
		WorldGenFlowers brownMushrooms(Tile::getMushroomBrownId());
		#ifdef ENABLE_DECORATION_DEBUG
		if (brownMushrooms.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8))
			mushroomsBrownPlaced++;
		#else
		brownMushrooms.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8);
		#endif
	}

	// Mushrooms (red) - 1/8 chance (ChunkProviderGenerate.java:470-475)
	if (random.nextInt(8) == 0)
	{
		WorldGenFlowers redMushrooms(Tile::getMushroomRedId());
		#ifdef ENABLE_DECORATION_DEBUG
		if (redMushrooms.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8))
			mushroomsRedPlaced++;
		#else
		redMushrooms.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8);
		#endif
	}

	// Reeds (sugar cane) - 10 attempts (ChunkProviderGenerate.java:477-482)
	#ifdef ENABLE_DECORATION_DEBUG
	int_t reedsPlaced = 0;
	#endif
	for (int_t i = 0; i < 10; ++i)
	{
		WorldGenReed reeds;
		#ifdef ENABLE_DECORATION_DEBUG
		if (reeds.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8))
			reedsPlaced++;
		#else
		reeds.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8);
		#endif
	}

	// TODO: Pumpkins - 1/32 chance (ChunkProviderGenerate.java:484-489)

	// Cactus - 10 attempts if desert biome (ChunkProviderGenerate.java:491-502)
	#ifdef ENABLE_DECORATION_DEBUG
	int_t cactusPlaced = 0;
	int_t oreBlocksPlaced = 0;
	#endif
	int_t cactusCount = 0;
	// Alpha: Check if chunk is in desert biome (MobSpawnerBase.desert) (ChunkProviderGenerate.java:492-493)
	// Check center of chunk for biome classification
	BiomeType centerBiome = level.getBiomeSource().getBiomeAt(cx + 8, cz + 8);
	if (BiomeSource::isDesert(centerBiome))
	{
		cactusCount = 10;  // Alpha: var16 += 10 if biome == desert
	}
	for (int_t i = 0; i < cactusCount; ++i)
	{
		WorldGenCactus cactus;
		#ifdef ENABLE_DECORATION_DEBUG
		if (cactus.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8))
			cactusPlaced++;
		#else
		cactus.generate(level, random, cx + random.nextInt(16) + 8, random.nextInt(128), cz + random.nextInt(16) + 8);
		#endif
	}

	// Water liquids - 50 attempts (ChunkProviderGenerate.java:504-509)
	int_t waterSpringAttempts = 0;
	int_t waterSpringSuccess = 0;
	for (int_t i = 0; i < 50; ++i)
	{
		int_t liquidX = cx + random.nextInt(16) + 8;
		int_t liquidY = random.nextInt(random.nextInt(120) + 8);  // Alpha: nested random.nextInt
		int_t liquidZ = cz + random.nextInt(16) + 8;
		WorldGenLiquids waterSpring(8);  // Alpha: Block.waterStill.blockID (ID 8)
		waterSpringAttempts++;
		bool success = waterSpring.generate(level, random, liquidX, liquidY, liquidZ);
		if (success)
			waterSpringSuccess++;
	}
	
	// Lava liquids - 20 attempts (ChunkProviderGenerate.java:511-516)
	for (int_t i = 0; i < 20; ++i)
	{
		int_t liquidX = cx + random.nextInt(16) + 8;
		int_t liquidY = random.nextInt(random.nextInt(random.nextInt(112) + 8) + 8);  // Alpha: triple nested random.nextInt
		int_t liquidZ = cz + random.nextInt(16) + 8;
		WorldGenLiquids lavaSpring(10);  // Alpha: Block.lavaStill.blockID (ID 10)
		lavaSpring.generate(level, random, liquidX, liquidY, liquidZ);
	}
	
	// Snow placement - Beta 1.2 (RandomLevelSource.java:602-619)
	// Beta: Get temperatures for chunk (16x16) using getTemperatureBlock()
	std::array<double, 16 * 16> &temperatures = level.getBiomeSource().getTemperatureBlock(cx + 8, cz + 8, 16, 16);
	
	for (int_t sx = cx + 8; sx < cx + 8 + 16; ++sx)
	{
		for (int_t sz = cz + 8; sz < cz + 8 + 16; ++sz)
		{
			// Beta: Calculate local index (RandomLevelSource.java:606-607)
			int_t var102 = sx - (cx + 8);
			int_t var105 = sz - (cz + 8);
			// Beta: Get temperature from array (RandomLevelSource.java:609)
			// Beta: Get top solid block (RandomLevelSource.java:608)
			int_t var20 = level.getTopSolidBlock(sx, sz);
			// Beta: Adjust temperature based on height (RandomLevelSource.java:609)
			double var21 = temperatures[var102 * 16 + var105] - (static_cast<double>(var20) - 64.0) / 64.0 * 0.3;
			// Beta: Place snow if conditions met (RandomLevelSource.java:610-616)
			if (var21 < 0.5 && var20 > 0 && var20 < 128 && level.isEmptyTile(sx, var20, sz) &&
			    level.getMaterial(sx, var20 - 1, sz).blocksMotion() &&
			    &level.getMaterial(sx, var20 - 1, sz) != &Material::ice)
			{
				level.setTileAndData(sx, var20, sz, Tile::snow.id, 0);  // Beta: Tile.topSnow.id (ID 78)
			}
		}
	}
	
	// Restore neighbor update flag
	level.noNeighborUpdate = oldNoNeighborUpdate;
	
	#ifdef ENABLE_DECORATION_DEBUG
	std::cout << "Decoration debug [chunk " << x << "," << z << "]: "
	          << "flowers(yellow=" << flowersYellowPlaced << ",red=" << flowersRedPlaced << "), "
	          << "mushrooms(brown=" << mushroomsBrownPlaced << ",red=" << mushroomsRedPlaced << "), "
	          << "reeds=" << reedsPlaced << ", cactus=" << cactusPlaced << ", ores=" << oreBlocksPlaced << std::endl;
	#endif
}

bool RandomLevelSource::save(bool force, std::shared_ptr<ProgressListener> progressListener)
{
	return true;
}

bool RandomLevelSource::tick()
{
	return false;
}

bool RandomLevelSource::shouldSave()
{
	return true;
}

jstring RandomLevelSource::gatherStats()
{
	return u"RandomLevelSource";
}
