#pragma once

#include "world/level/chunk/ChunkSource.h"

#include <array>
#include <vector>

#include "world/level/levelgen/synth/PerlinNoise.h"
#include "world/level/levelgen/HellCaveFeature.h"

#include "java/Random.h"

class Level;

class HellRandomLevelSource : public ChunkSource
{
public:
	static constexpr int_t CHUNK_HEIGHT = 8;
	static constexpr int_t CHUNK_WIDTH = 4;

private:
	Random random; // Alpha: field_4170_h

	PerlinNoise lperlinNoise1; // Alpha: field_4169_i
	PerlinNoise lperlinNoise2; // Alpha: field_4168_j
	PerlinNoise perlinNoise1;  // Alpha: field_4167_k
	PerlinNoise perlinNoise2;  // Alpha: field_4166_l
	PerlinNoise perlinNoise3;  // Alpha: field_4165_m
	PerlinNoise scaleNoise;    // Alpha: field_4177_a
	PerlinNoise depthNoise;    // Alpha: field_4176_b

	Level &level; // Alpha: field_4164_n

	std::vector<double> buffer; // Alpha: field_4163_o

	std::array<double, 16 * 16> soulSandBuffer = {}; // Alpha: field_4162_p
	std::array<double, 16 * 16> gravelBuffer = {};   // Alpha: field_4161_q
	std::array<double, 16 * 16> depthBuffer = {};    // Alpha: field_4160_r

	HellCaveFeature caveFeature; // Alpha: field_4159_s

	std::vector<double> pnr; // Alpha: field_4175_c
	std::vector<double> ar;  // Alpha: field_4174_d
	std::vector<double> br;  // Alpha: field_4173_e
	std::vector<double> sr;  // Alpha: field_4172_f
	std::vector<double> dr;  // Alpha: field_4171_g

	// Alpha allocates the cosine ramp fresh on every getHeights() call; every
	// entry is written before it is read, so a persistent buffer is equivalent.
	std::vector<double> ramp;

public:
	HellRandomLevelSource(Level &level, long_t seed);

	void prepareHeights(int_t x, int_t z, ubyte_t *tiles);
	void buildSurfaces(int_t x, int_t z, ubyte_t *tiles);
	std::shared_ptr<LevelChunk> getChunk(int_t x, int_t z) override;
private:
	void getHeights(double *out, int_t x, int_t y, int_t z, int_t xSize, int_t ySize, int_t zSize);
public:
	bool hasChunk(int_t x, int_t z) override;
	void postProcess(ChunkSource &parent, int_t x, int_t z) override;
	bool save(bool force, std::shared_ptr<ProgressListener> progressListener) override;
	bool tick() override;
	bool shouldSave() override;
	jstring gatherStats() override;
};
