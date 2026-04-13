#pragma once

#include <vector>
#include <array>
#include <memory>

#include "world/level/LevelSource.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"

// A thread-safe snapshot of world data for a region of chunks.
// Copies all block/light/heightmap data so it can be read safely from a worker thread.
class ChunkSnapshot : public LevelSource
{
private:
	// Region bounds in chunk coordinates
	int_t xc1 = 0, zc1 = 0;
	int_t chunksW = 0, chunksH = 0;

	// Per-chunk copied data (indexed by (xc - xc1) * chunksH + (zc - zc1))
	struct ChunkData
	{
		std::array<ubyte_t, 16 * 128 * 16> blocks = {};
		std::array<byte_t, 16 * 128 * 16 / 2> data = {};
		std::array<byte_t, 16 * 128 * 16 / 2> skyLight = {};
		std::array<byte_t, 16 * 128 * 16 / 2> blockLight = {};
		std::array<ubyte_t, 16 * 16> heightmap = {};
		std::unordered_map<int_t, std::shared_ptr<TileEntity>> tileEntities;
		bool valid = false;
	};

	std::vector<ChunkData> chunkData;
	int_t skyDarken = 0;
	float brightnessRamp[16] = {};

	// Dummy BiomeSource for getBiomeSource() (not used during meshing)
	BiomeSource *biomeSourcePtr = nullptr;

	int_t getDataNibble(const std::array<byte_t, 16 * 128 * 16 / 2> &arr, int_t x, int_t y, int_t z) const;

public:
	// Construct a snapshot covering the region needed to mesh a render chunk.
	// The render chunk is at world coords (rx, ry, rz) with size 16.
	// We copy a 1-block border around it (so +-1 in all directions).
	ChunkSnapshot(Level &level, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);

	// LevelSource interface
	int_t getTile(int_t x, int_t y, int_t z) override;
	std::shared_ptr<TileEntity> getTileEntity(int_t x, int_t y, int_t z) override;
	float getBrightness(int_t x, int_t y, int_t z) override;
	int_t getData(int_t x, int_t y, int_t z) override;
	const Material &getMaterial(int_t x, int_t y, int_t z) override;
	bool isSolidTile(int_t x, int_t y, int_t z) override;
	BiomeSource &getBiomeSource() override;

	// Additional methods matching Region's interface used by TileRenderer
	int_t getRawBrightness(int_t x, int_t y, int_t z);
	int_t getRawBrightness(int_t x, int_t y, int_t z, bool neighbors);

	// touchedSky tracking (per-snapshot, not static)
	bool touchedSky = false;
};
