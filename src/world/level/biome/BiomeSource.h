#pragma once

#include "world/level/levelgen/synth/PerlinSimplexNoise.h"

#include "java/Type.h"

class Level;

// Alpha: MobSpawnerBase biome types (MobSpawnerBase.java:6-17)
enum class BiomeType
{
	RAINFOREST,
	SWAMPLAND,
	SEASONAL_FOREST,
	FOREST,
	SAVANNA,
	SHRUBLAND,
	TAIGA,
	DESERT,
	PLAINS,
	ICE_DESERT,
	TUNDRA,
	HELL
};

class BiomeSource
{
private:
	PerlinSimplexNoise temperatureMap;
	PerlinSimplexNoise downfallMap;
	PerlinSimplexNoise noiseMap;

public:
	std::array<double, 16 * 16> temperatures = {};
	std::array<double, 16 * 16> downfalls = {};
	std::array<double, 16 * 16> noises = {};

private:
	static constexpr float zoom = 2.0f;
	static constexpr float tempScale = 0.025f;
	static constexpr float downfallScale = 0.05f;
	static constexpr float noiseScale = 0.25f;

public:
	BiomeSource(Level &level);

	double getTemperature(int_t x, int_t z);

	void getBiomeBlock(int_t x, int_t z, int_t xd, int_t zd);
	
	// Beta: BiomeSource.getTemperatureBlock() - returns temperature array for snow generation
	std::array<double, 16 * 16> &getTemperatureBlock(int_t x, int_t z, int_t xd, int_t zd);
	
	// Alpha: MobSpawnerBase.getBiome(float var0, float var1) (MobSpawnerBase.java:63-65)
	// var0 = temperature, var1 = rainfall/downfall
	// Returns biome type based on Alpha's classification logic
	static BiomeType getBiome(float temperature, float downfall);
	
	// Alpha: Cold biomes are tundra, taiga, iceDesert (those that call func_4122_b())
	static bool isCold(BiomeType biome);
	
	// Alpha: Desert biomes are desert, savanna, plains, iceDesert (MobSpawnerDesert instances)
	static bool isDesert(BiomeType biome);
	
	// Get biome at specific coordinates (uses temperature/downfall from arrays)
	BiomeType getBiomeAt(int_t x, int_t z);
};
