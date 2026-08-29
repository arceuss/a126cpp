#pragma once

#include "world/level/biome/BiomeSource.h"

// Alpha: WorldChunkManagerHell answers one biome, one temperature and one
// downfall for every position instead of sampling noise
// (WorldChunkManagerHell.java:17-58). The nether uses it with MobSpawnerBase.hell,
// temperature 1.0 and downfall 0.0 (WorldProviderHell.java:20).
class FixedBiomeSource : public BiomeSource
{
private:
	BiomeType biome;
	double temperature;
	double downfall;

public:
	FixedBiomeSource(Level &level, BiomeType biome, double temperature, double downfall);

	double getTemperature(int_t x, int_t z) override;
	void getBiomeBlock(int_t x, int_t z, int_t xd, int_t zd) override;
	std::array<double, 16 * 16> &getTemperatureBlock(int_t x, int_t z, int_t xd, int_t zd) override;
	BiomeType getBiomeAt(int_t x, int_t z) override;
};
