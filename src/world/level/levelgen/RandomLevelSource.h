#pragma once

#include "world/level/chunk/ChunkSource.h"

#include <array>

#include "world/level/levelgen/synth/PerlinNoise.h"
#include "world/level/levelgen/LargeCaveFeature.h"

#include "java/Random.h"

class Level;
class BiomeSource;

// 4J: Scratch buffers moved to per-task struct for thread safety
// "used to be declared with class level scope but moved here for thread safety"
struct ChunkGenScratch
{
	static constexpr int_t CHUNK_HEIGHT = 8;
	static constexpr int_t CHUNK_WIDTH = 4;
	static constexpr int_t BUFFER_WIDTH = 16 / CHUNK_WIDTH;
	static constexpr int_t BUFFER_HEIGHT = 128 / CHUNK_HEIGHT;
	static constexpr int_t BUFFER_WIDTH_1 = BUFFER_WIDTH + 1;
	static constexpr int_t BUFFER_HEIGHT_1 = BUFFER_HEIGHT + 1;

	std::array<double, 16 * 16> sandBuffer = {};
	std::array<double, 16 * 16> gravelBuffer = {};
	std::array<double, 16 * 16> depthBuffer = {};

	std::array<double, BUFFER_WIDTH_1 * BUFFER_WIDTH_1 * BUFFER_HEIGHT_1> buffer = {};
	std::array<double, BUFFER_WIDTH_1 * BUFFER_WIDTH_1 * BUFFER_HEIGHT_1> pnr = {};
	std::array<double, BUFFER_WIDTH_1 * BUFFER_WIDTH_1 * BUFFER_HEIGHT_1> ar = {};
	std::array<double, BUFFER_WIDTH_1 * BUFFER_WIDTH_1 * BUFFER_HEIGHT_1> br = {};
	std::array<double, BUFFER_WIDTH_1 * BUFFER_WIDTH_1> sr = {};
	std::array<double, BUFFER_WIDTH_1 * BUFFER_WIDTH_1> dr = {};

	// 4J: "created locally here for thread safety, java has this as a class member"
	std::array<double, 16 * 16> temperatures = {};
	std::array<double, 16 * 16> downfalls = {};
	std::array<double, 16 * 16> noises = {};
};

class RandomLevelSource : public ChunkSource
{
private:
	static constexpr double SNOW_CUTOFF = 0.5;
	static constexpr double SNOW_SCALE = 0.3;
	static constexpr bool FLOATING_ISLANDS = false;

public:
	static constexpr int_t CHUNK_HEIGHT = 8;
	static constexpr int_t CHUNK_WIDTH = 4;

private:
	Random random;
	// 4J: "added, so that we can have a separate random for doing post-processing in parallel with creation"
	Random pprandom;

	// Noise generators are immutable after construction - thread-safe for concurrent reads
	PerlinNoise lperlinNoise1;
	PerlinNoise lperlinNoise2;
	PerlinNoise perlinNoise1;
	PerlinNoise perlinNoise2;
	PerlinNoise perlinNoise3;
	PerlinNoise scaleNoise;
	PerlinNoise depthNoise;
	PerlinNoise forestNoise;

	Level &level;

	static constexpr int_t BUFFER_WIDTH = 16 / CHUNK_WIDTH;
	static constexpr int_t BUFFER_HEIGHT = 128 / CHUNK_HEIGHT;
	static constexpr int_t BUFFER_WIDTH_1 = BUFFER_WIDTH + 1;
	static constexpr int_t BUFFER_HEIGHT_1 = BUFFER_HEIGHT + 1;

public:
	RandomLevelSource(Level &level, long_t seed);

	// Thread-safe terrain generation: all scratch data in ChunkGenScratch
	void prepareHeights(int_t x, int_t z, ubyte_t *tiles, double *temperatures, ChunkGenScratch &scratch);
	void buildSurfaces(int_t x, int_t z, ubyte_t *tiles, Random &rng, ChunkGenScratch &scratch);
	void getHeights(double *out, int_t x, int_t y, int_t z, int_t xd, int_t yd, int_t zd, double *temperatures, double *downfalls, ChunkGenScratch &scratch);

	// Fill biome arrays in scratch buffer (thread-safe: reads immutable noise gens, writes to scratch)
	void getBiomeBlock(BiomeSource &biomeSource, int_t x, int_t z, int_t xd, int_t zd, ChunkGenScratch &scratch);

	// Generate terrain for a chunk (thread-safe with per-task scratch/rng/cave)
	std::shared_ptr<LevelChunk> generateTerrain(int_t x, int_t z, Random &rng, ChunkGenScratch &scratch, LargeCaveFeature &caveFeature);

	std::shared_ptr<LevelChunk> getChunk(int_t x, int_t z) override;

	bool hasChunk(int_t x, int_t z) override;
private:
	void calcWaterDepths(std::shared_ptr<ChunkSource> parent, int_t x, int_t z);
public:
	void postProcess(ChunkSource &parent, int_t x, int_t z) override;
	bool save(bool force, std::shared_ptr<ProgressListener> progressListener) override;
	bool tick() override;
	bool shouldSave() override;
	jstring gatherStats() override;
};
