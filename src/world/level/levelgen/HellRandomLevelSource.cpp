#include "world/level/levelgen/HellRandomLevelSource.h"

#include <cmath>

#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"

#include "world/level/tile/Tile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/GravelTile.h"
#include "world/level/tile/UnbreakableTile.h"
#include "world/level/tile/MushroomTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/HellStoneTile.h"
#include "world/level/tile/HellSandTile.h"

#include "world/level/levelgen/feature/WorldGenHellLava.h"
#include "world/level/levelgen/feature/WorldGenFire.h"
#include "world/level/levelgen/feature/WorldGenLightStone1.h"
#include "world/level/levelgen/feature/WorldGenLightStone2.h"
#include "world/level/levelgen/feature/WorldGenFlowers.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

HellRandomLevelSource::HellRandomLevelSource(Level &level, long_t seed) :
	random(seed),
	lperlinNoise1(random, 16),
	lperlinNoise2(random, 16),
	perlinNoise1(random, 8),
	perlinNoise2(random, 4),
	perlinNoise3(random, 4),
	scaleNoise(random, 10),
	depthNoise(random, 16),
	level(level)
{

}

void HellRandomLevelSource::prepareHeights(int_t x, int_t z, ubyte_t *tiles)
{
	int_t width = CHUNK_WIDTH;
	int_t lavaLevel = 32; // Alpha: everything below y=32 starts as the lava sea (ChunkProviderHell.java:52)
	int_t xSize = width + 1;
	int_t ySize = 17;
	int_t zSize = width + 1;

	if (buffer.size() < static_cast<size_t>(xSize * ySize * zSize))
		buffer.resize(static_cast<size_t>(xSize * ySize * zSize));

	getHeights(buffer.data(), x * width, 0, z * width, xSize, ySize, zSize);

	for (int_t xi = 0; xi < width; xi++)
	{
		for (int_t zi = 0; zi < width; zi++)
		{
			for (int_t yi = 0; yi < 16; yi++)
			{
				double ddiv = 0.125;

				double v000 = buffer[((xi + 0) * zSize + (zi + 0)) * ySize + (yi + 0)];
				double v010 = buffer[((xi + 0) * zSize + (zi + 1)) * ySize + (yi + 0)];
				double v100 = buffer[((xi + 1) * zSize + (zi + 0)) * ySize + (yi + 0)];
				double v110 = buffer[((xi + 1) * zSize + (zi + 1)) * ySize + (yi + 0)];
				double d001 = (buffer[((xi + 0) * zSize + (zi + 0)) * ySize + (yi + 1)] - v000) * ddiv;
				double d011 = (buffer[((xi + 0) * zSize + (zi + 1)) * ySize + (yi + 1)] - v010) * ddiv;
				double d101 = (buffer[((xi + 1) * zSize + (zi + 0)) * ySize + (yi + 1)] - v100) * ddiv;
				double d111 = (buffer[((xi + 1) * zSize + (zi + 1)) * ySize + (yi + 1)] - v110) * ddiv;

				for (int_t cyi = 0; cyi < CHUNK_HEIGHT; cyi++)
				{
					double ddiv2 = 0.25;

					double vx00 = v000;
					double vx10 = v010;
					double dx00 = (v100 - v000) * ddiv2;
					double dx10 = (v110 - v010) * ddiv2;

					for (int_t cxi = 0; cxi < CHUNK_WIDTH; cxi++)
					{
						int_t i = ((cxi + xi * CHUNK_WIDTH) << 11) | ((zi * CHUNK_WIDTH) << 7) | (yi * CHUNK_HEIGHT + cyi);
						int_t pitch = 128;

						double ddiv3 = 0.25;

						double vxx0 = vx00;
						double dxx0 = (vx10 - vx00) * ddiv3;

						for (int_t czi = 0; czi < CHUNK_WIDTH; czi++)
						{
							int_t tile = 0;

							// Alpha: Block.lavaMoving is the stationary lava, id 11 (Block.java:106)
							if (yi * CHUNK_HEIGHT + cyi < lavaLevel)
								tile = Tile::lavaMoving.id;

							if (vxx0 > 0.0)
								tile = Tile::hellRock.id;

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

void HellRandomLevelSource::buildSurfaces(int_t x, int_t z, ubyte_t *tiles)
{
	int_t seaLevel = 64; // Alpha: n3 = 64 (ChunkProviderHell.java:113)

	double scale = 1.0 / 32.0;
	perlinNoise2.getRegion(soulSandBuffer.data(), x * 16, z * 16, 0.0, 16, 16, 1, scale, scale, 1.0);
	perlinNoise2.getRegion(gravelBuffer.data(), z * 16, 109.0134, x * 16, 16, 1, 16, scale, 1.0, scale);
	perlinNoise3.getRegion(depthBuffer.data(), x * 16, z * 16, 0.0, 16, 16, 1, scale * 2.0, scale * 2.0, scale * 2.0);

	for (int_t xi = 0; xi < 16; xi++)
	{
		for (int_t zi = 0; zi < 16; zi++)
		{
			bool isSoulSand = (soulSandBuffer[xi + zi * 16] + random.nextDouble() * 0.2) > 0.0;
			bool isGravel = (gravelBuffer[xi + zi * 16] + random.nextDouble() * 0.2) > 0.0;

			int_t depth = static_cast<int_t>(depthBuffer[xi + zi * 16] / 3.0 + 3.0 + random.nextDouble() * 0.25);
			int_t depthI = -1;

			int_t topTile = Tile::hellRock.id;
			int_t fillerTile = Tile::hellRock.id;

			for (int_t y = 127; y >= 0; y--)
			{
				int_t i = (xi * 16 + zi) * 128 + y;

				// Alpha: bedrock roof, one nextInt(5) draw per column-cell (ChunkProviderHell.java:126-129)
				if (y >= 127 - random.nextInt(5))
				{
					tiles[i] = Tile::unbreakable.id;
					continue;
				}

				// Alpha: bedrock floor (ChunkProviderHell.java:130-133)
				if (y <= 0 + random.nextInt(5))
				{
					tiles[i] = Tile::unbreakable.id;
					continue;
				}

				int_t oldTile = tiles[i];
				if (oldTile == 0)
				{
					depthI = -1;
					continue;
				}

				if (oldTile != Tile::hellRock.id)
					continue;

				if (depthI == -1)
				{
					if (depth <= 0)
					{
						topTile = 0;
						fillerTile = Tile::hellRock.id;
					}
					else if (y >= seaLevel - 4 && y <= seaLevel + 1)
					{
						topTile = Tile::hellRock.id;
						fillerTile = Tile::hellRock.id;

						if (isGravel) topTile = Tile::gravel.id;
						if (isGravel) fillerTile = Tile::hellRock.id;
						if (isSoulSand) topTile = Tile::hellSand.id;
						if (isSoulSand) fillerTile = Tile::hellSand.id;
					}

					// Alpha: Block.lavaMoving is the stationary lava, id 11 (Block.java:106)
					if (y < seaLevel && topTile == 0)
						topTile = Tile::lavaMoving.id;

					depthI = depth;
					if (y >= seaLevel - 1)
						tiles[i] = topTile;
					else
						tiles[i] = fillerTile;
					continue;
				}

				if (depthI <= 0)
					continue;

				depthI--;
				tiles[i] = fillerTile;
			}
		}
	}
}

std::shared_ptr<LevelChunk> HellRandomLevelSource::getChunk(int_t x, int_t z)
{
	random.setSeed(x * 341873128712LL + z * 132897987541LL);

	std::shared_ptr<LevelChunk> chunk = Util::make_shared<LevelChunk>(level, x, z);

	prepareHeights(x, z, chunk->blocks.data());

	buildSurfaces(x, z, chunk->blocks.data());

	caveFeature.apply(*this, level, x, z, chunk->blocks);

	chunk->recalcHeightmap();
	chunk->lightLava();

	return chunk;
}

void HellRandomLevelSource::getHeights(double *out, int_t x, int_t y, int_t z, int_t xSize, int_t ySize, int_t zSize)
{
	double xzScale = 684.412;
	double yScale = 2053.236;

	size_t size2d = static_cast<size_t>(xSize * zSize);
	size_t size3d = static_cast<size_t>(xSize * ySize * zSize);

	if (sr.size() < size2d) sr.resize(size2d);
	if (dr.size() < size2d) dr.resize(size2d);
	if (pnr.size() < size3d) pnr.resize(size3d);
	if (ar.size() < size3d) ar.resize(size3d);
	if (br.size() < size3d) br.resize(size3d);
	if (ramp.size() < static_cast<size_t>(ySize)) ramp.resize(static_cast<size_t>(ySize));

	scaleNoise.getRegion(sr.data(), x, y, z, xSize, 1, zSize, 1.0, 0.0, 1.0);
	depthNoise.getRegion(dr.data(), x, y, z, xSize, 1, zSize, 100.0, 0.0, 100.0);
	perlinNoise1.getRegion(pnr.data(), x, y, z, xSize, ySize, zSize, xzScale / 80.0, yScale / 60.0, xzScale / 80.0);
	lperlinNoise1.getRegion(ar.data(), x, y, z, xSize, ySize, zSize, xzScale, yScale, xzScale);
	lperlinNoise2.getRegion(br.data(), x, y, z, xSize, ySize, zSize, xzScale, yScale, xzScale);

	int_t i3 = 0;
	int_t i2 = 0;

	// Alpha: cosine ramp carves the roof and floor slabs out of the noise field,
	// dampened over the outermost 4 cells at each end (ChunkProviderHell.java:227-237)
	for (int_t yi = 0; yi < ySize; yi++)
	{
		ramp[yi] = std::cos(static_cast<double>(yi) * M_PI * 6.0 / static_cast<double>(ySize)) * 2.0;

		double edge = yi;
		if (yi > ySize / 2)
			edge = ySize - 1 - yi;

		if (edge < 4.0)
		{
			edge = 4.0 - edge;
			ramp[yi] = ramp[yi] - edge * edge * edge * 10.0;
		}
	}

	for (int_t xi = 0; xi < xSize; xi++)
	{
		for (int_t zi = 0; zi < zSize; zi++)
		{
			// Alpha computes sv and dv here but never applies either to a column,
			// so the nether gets no height term at all (ChunkProviderHell.java:240-268)
			double sv = (sr[i2] + 256.0) / 512.0;
			if (sv > 1.0)
				sv = 1.0;

			double minHeight = 0.0;

			double dv = dr[i2] / 8000.0;
			if (dv < 0.0)
				dv = -dv;
			dv = dv * 3.0 - 3.0;

			if (dv < 0.0)
			{
				dv /= 2.0;
				if (dv < -1.0)
					dv = -1.0;
				dv /= 1.4;
				dv /= 2.0;
				sv = 0.0;
			}
			else
			{
				if (dv > 1.0)
					dv = 1.0;
				dv /= 6.0;
			}

			sv += 0.5;
			dv = dv * static_cast<double>(ySize) / 16.0;

			i2++;

			for (int_t yi = 0; yi < ySize; yi++)
			{
				double final = 0.0;

				double rampValue = ramp[yi];
				double av = ar[i3] / 512.0;
				double bv = br[i3] / 512.0;
				double pnv = (pnr[i3] / 10.0 + 1.0) / 2.0;

				if (pnv < 0.0)
					final = av;
				else if (pnv > 1.0)
					final = bv;
				else
					final = av + (bv - av) * pnv;

				final -= rampValue;

				if (yi > ySize - 4)
				{
					// Alpha divides in float here, not double (ChunkProviderHell.java:283)
					double factor = static_cast<float>(yi - (ySize - 4)) / 3.0f;
					final = final * (1.0 - factor) + -10.0 * factor;
				}

				if (static_cast<double>(yi) < minHeight)
				{
					double factor = (minHeight - static_cast<double>(yi)) / 4.0;
					if (factor < 0.0)
						factor = 0.0;
					if (factor > 1.0)
						factor = 1.0;
					final = final * (1.0 - factor) + -10.0 * factor;
				}

				out[i3] = final;
				i3++;
			}
		}
	}
}

bool HellRandomLevelSource::hasChunk(int_t x, int_t z)
{
	return true;
}

void HellRandomLevelSource::postProcess(ChunkSource &parent, int_t x, int_t z)
{
	SandTile::fallInstantly = true;

	int_t xx = x * 16;
	int_t zz = z * 16;

	for (int_t i = 0; i < 8; i++)
	{
		int_t px = xx + random.nextInt(16) + 8;
		int_t py = random.nextInt(120) + 4;
		int_t pz = zz + random.nextInt(16) + 8;
		// Alpha: Block.lavaStill is the flowing lava, id 10 (Block.java:105)
		WorldGenHellLava(Tile::lavaStill.id).generate(level, random, px, py, pz);
	}

	int_t count = random.nextInt(random.nextInt(10) + 1) + 1;
	for (int_t i = 0; i < count; i++)
	{
		int_t px = xx + random.nextInt(16) + 8;
		int_t py = random.nextInt(120) + 4;
		int_t pz = zz + random.nextInt(16) + 8;
		WorldGenFire().generate(level, random, px, py, pz);
	}

	count = random.nextInt(random.nextInt(10) + 1);
	for (int_t i = 0; i < count; i++)
	{
		int_t px = xx + random.nextInt(16) + 8;
		int_t py = random.nextInt(120) + 4;
		int_t pz = zz + random.nextInt(16) + 8;
		WorldGenLightStone1().generate(level, random, px, py, pz);
	}

	for (int_t i = 0; i < 10; i++)
	{
		int_t px = xx + random.nextInt(16) + 8;
		int_t py = random.nextInt(128);
		int_t pz = zz + random.nextInt(16) + 8;
		WorldGenLightStone2().generate(level, random, px, py, pz);
	}

	// Alpha: nextInt(1) is always 0, but the draw still advances the stream (ChunkProviderHell.java:316)
	if (random.nextInt(1) == 0)
	{
		int_t px = xx + random.nextInt(16) + 8;
		int_t py = random.nextInt(128);
		int_t pz = zz + random.nextInt(16) + 8;
		WorldGenFlowers(Tile::mushroomBrown.id).generate(level, random, px, py, pz);
	}

	if (random.nextInt(1) == 0)
	{
		int_t px = xx + random.nextInt(16) + 8;
		int_t py = random.nextInt(128);
		int_t pz = zz + random.nextInt(16) + 8;
		WorldGenFlowers(Tile::mushroomRed.id).generate(level, random, px, py, pz);
	}

	SandTile::fallInstantly = false;
}

bool HellRandomLevelSource::save(bool force, std::shared_ptr<ProgressListener> progressListener)
{
	return true;
}

bool HellRandomLevelSource::tick()
{
	return false;
}

bool HellRandomLevelSource::shouldSave()
{
	return true;
}

jstring HellRandomLevelSource::gatherStats()
{
	return u"HellRandomLevelSource";
}
