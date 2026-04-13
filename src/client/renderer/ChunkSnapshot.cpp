#include "client/renderer/ChunkSnapshot.h"

#include <cstring>

#include "world/level/material/GasMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/tile/StoneSlabTile.h"

ChunkSnapshot::ChunkSnapshot(Level &level, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	xc1 = x0 >> 4;
	zc1 = z0 >> 4;
	int_t xc2 = x1 >> 4;
	int_t zc2 = z1 >> 4;

	chunksW = xc2 - xc1 + 1;
	chunksH = zc2 - zc1 + 1;

	chunkData.resize(chunksW * chunksH);

	skyDarken = level.skyDarken;
	std::memcpy(brightnessRamp, level.dimension->brightnessRamp, sizeof(brightnessRamp));
	biomeSourcePtr = &level.getBiomeSource();

	for (int_t xc = xc1; xc <= xc2; xc++)
	{
		for (int_t zc = zc1; zc <= zc2; zc++)
		{
			auto chunk = level.getChunk(xc, zc);
			if (chunk == nullptr) continue;

			int_t idx = (xc - xc1) * chunksH + (zc - zc1);
			ChunkData &cd = chunkData[idx];
			cd.valid = true;

			// Copy block and lighting data
			std::memcpy(cd.blocks.data(), chunk->blocks.data(), cd.blocks.size());
			std::memcpy(cd.data.data(), chunk->data.data.data(), cd.data.size());
			std::memcpy(cd.skyLight.data(), chunk->skyLight.data.data(), cd.skyLight.size());
			std::memcpy(cd.blockLight.data(), chunk->blockLight.data.data(), cd.blockLight.size());
			std::memcpy(cd.heightmap.data(), chunk->heightmap.data(), cd.heightmap.size());

			// Copy tile entity pointers
			for (auto &pair : chunk->tileEntities)
			{
				int_t key = (pair.first.x & 0xF) | ((pair.first.z & 0xF) << 4) | (pair.first.y << 8);
				cd.tileEntities[key] = pair.second;
			}
		}
	}
}

int_t ChunkSnapshot::getDataNibble(const std::array<byte_t, 16 * 128 * 16 / 2> &arr, int_t x, int_t y, int_t z) const
{
	int_t idx = (x * 16 + z) * 128 + y;
	int_t byteIdx = idx >> 1;
	if (idx & 1)
		return (arr[byteIdx] >> 4) & 0xF;
	else
		return arr[byteIdx] & 0xF;
}

int_t ChunkSnapshot::getTile(int_t x, int_t y, int_t z)
{
	if (y < 0 || y >= Level::DEPTH)
		return 0;

	int_t xc = (x >> 4) - xc1;
	int_t zc = (z >> 4) - zc1;
	if (xc < 0 || xc >= chunksW || zc < 0 || zc >= chunksH)
		return 0;

	ChunkData &cd = chunkData[xc * chunksH + zc];
	if (!cd.valid) return 0;

	int_t lx = x & 0xF;
	int_t lz = z & 0xF;
	return cd.blocks[(lx * 16 + lz) * 128 + y];
}

std::shared_ptr<TileEntity> ChunkSnapshot::getTileEntity(int_t x, int_t y, int_t z)
{
	int_t xc = (x >> 4) - xc1;
	int_t zc = (z >> 4) - zc1;
	if (xc < 0 || xc >= chunksW || zc < 0 || zc >= chunksH)
		return nullptr;

	ChunkData &cd = chunkData[xc * chunksH + zc];
	if (!cd.valid) return nullptr;

	int_t key = (x & 0xF) | ((z & 0xF) << 4) | (y << 8);
	auto it = cd.tileEntities.find(key);
	if (it != cd.tileEntities.end())
		return it->second;
	return nullptr;
}

float ChunkSnapshot::getBrightness(int_t x, int_t y, int_t z)
{
	return brightnessRamp[getRawBrightness(x, y, z)];
}

int_t ChunkSnapshot::getRawBrightness(int_t x, int_t y, int_t z)
{
	return getRawBrightness(x, y, z, true);
}

int_t ChunkSnapshot::getRawBrightness(int_t x, int_t y, int_t z, bool neighbors)
{
	if (x < -Level::MAX_LEVEL_SIZE || z < -Level::MAX_LEVEL_SIZE || x >= Level::MAX_LEVEL_SIZE || z >= Level::MAX_LEVEL_SIZE)
		return 15;

	if (neighbors)
	{
		int_t tile = getTile(x, y, z);
		if (tile == Tile::stoneSlabHalf.id || tile == Tile::farmland.id)
		{
			int_t brightness = getRawBrightness(x, y + 1, z, false);
			int_t bpx = getRawBrightness(x + 1, y, z, false);
			int_t bnx = getRawBrightness(x - 1, y, z, false);
			int_t bpz = getRawBrightness(x, y, z + 1, false);
			int_t bnz = getRawBrightness(x, y, z - 1, false);

			if (bpx > brightness) brightness = bpx;
			if (bnx > brightness) brightness = bnx;
			if (bpz > brightness) brightness = bpz;
			if (bnz > brightness) brightness = bnz;

			return brightness;
		}
	}

	if (y < 0) return 0;
	if (y >= Level::DEPTH)
	{
		int_t l = 15 - skyDarken;
		if (l < 0) l = 0;
		return l;
	}

	int_t xc = (x >> 4) - xc1;
	int_t zc = (z >> 4) - zc1;
	if (xc < 0 || xc >= chunksW || zc < 0 || zc >= chunksH)
		return 0;

	ChunkData &cd = chunkData[xc * chunksH + zc];
	if (!cd.valid) return 0;

	int_t lx = x & 0xF;
	int_t lz = z & 0xF;

	int_t sky = getDataNibble(cd.skyLight, lx, y, lz);
	if (sky > 0) touchedSky = true;
	sky -= skyDarken;

	int_t block = getDataNibble(cd.blockLight, lx, y, lz);
	if (block > sky) sky = block;
	return sky;
}

int_t ChunkSnapshot::getData(int_t x, int_t y, int_t z)
{
	if (y < 0 || y >= Level::DEPTH)
		return 0;

	int_t xc = (x >> 4) - xc1;
	int_t zc = (z >> 4) - zc1;
	if (xc < 0 || xc >= chunksW || zc < 0 || zc >= chunksH)
		return 0;

	ChunkData &cd = chunkData[xc * chunksH + zc];
	if (!cd.valid) return 0;

	return getDataNibble(cd.data, x & 0xF, y, z & 0xF);
}

const Material &ChunkSnapshot::getMaterial(int_t x, int_t y, int_t z)
{
	int_t tile = getTile(x, y, z);
	return (tile == 0) ? Material::air : Tile::tiles[tile]->material;
}

bool ChunkSnapshot::isSolidTile(int_t x, int_t y, int_t z)
{
	Tile *tile = Tile::tiles[getTile(x, y, z)];
	return (tile == nullptr) ? false : tile->isSolidRender();
}

BiomeSource &ChunkSnapshot::getBiomeSource()
{
	return *biomeSourcePtr;
}
