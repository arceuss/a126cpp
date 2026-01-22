#include "world/level/biome/BiomeSource.h"

#include "world/level/Level.h"

BiomeSource::BiomeSource(Level &level) :
	temperatureMap(Random(level.seed * 9871LL), 4),
	downfallMap(Random(level.seed * 39811LL), 4),
	noiseMap(Random(level.seed * 543321LL), 2)
{
	
}

double BiomeSource::getTemperature(int_t x, int_t z)
{
	temperatureMap.getRegion(temperatures.data(), x, z, 1, 1, 0.025, 0.025, 0.5);
	return temperatures[0];
}

void BiomeSource::getBiomeBlock(int_t x, int_t z, int_t xd, int_t zd)
{
	temperatureMap.getRegion(temperatures.data(), x, z, xd, xd, 0.025, 0.025, 1.0 / 4.0);
	downfallMap.getRegion(downfalls.data(), x, z, xd, xd, 0.05, 0.05, 1.0 / 3.0);
	noiseMap.getRegion(noises.data(), x, z, xd, xd, 0.25, 0.25, 1.0 / 1.7);

	int_t i2 = 0;

	for (int_t xi = 0; xi < xd; xi++)
	{
		for (int_t zi = 0; zi < zd; zi++)
		{
			double nv = noises[i2] * 1.1 + 0.5;

			double d2 = 0.01;
			double d3 = 1.0 - d2;

			double temperature = (temperatures[i2] * 0.15 + 0.7) * d3 + nv * d2;

			d2 = 0.002;
			d3 = 1.0 - d2;

			double downfall = (downfalls[i2] * 0.15 + 0.5) * d3 + nv * d2;

			temperature = 1.0 - (1.0 - temperature) * (1.0 - temperature);

			if (temperature < 0.0) temperature = 0.0;
			if (downfall < 0.0) downfall = 0.0;
			if (temperature > 1.0) temperature = 1.0;
			if (downfall > 1.0) downfall = 1.0;

			temperatures[i2] = temperature;
			downfalls[i2] = downfall;

			i2++;
		}
	}
}

// Alpha: MobSpawnerBase.getBiome(float var0, float var1) (MobSpawnerBase.java:63-65)
// var0 = temperature, var1 = rainfall/downfall
// Exact 1:1 port of Alpha's biome classification logic
BiomeType BiomeSource::getBiome(float temperature, float downfall)
{
	// Alpha: var1 *= var0 (rainfall multiplied by temperature)
	downfall *= temperature;
	
	// Alpha: Exact classification tree from MobSpawnerBase.getBiome()
	// return var0 < 0.1F ? tundra : (var1 < 0.2F ? (var0 < 0.5F ? tundra : (var0 < 0.95F ? savanna : desert)) : (var1 > 0.5F && var0 < 0.7F ? swampland : (var0 < 0.5F ? taiga : (var0 < 0.97F ? (var1 < 0.35F ? shrubland : forest) : (var1 < 0.45F ? plains : (var1 < 0.9F ? seasonalForest : rainforest))))));
	if (temperature < 0.1f)
		return BiomeType::TUNDRA;
	
	if (downfall < 0.2f)
	{
		if (temperature < 0.5f)
			return BiomeType::TUNDRA;
		else if (temperature < 0.95f)
			return BiomeType::SAVANNA;
		else
			return BiomeType::DESERT;
	}
	
	if (downfall > 0.5f && temperature < 0.7f)
		return BiomeType::SWAMPLAND;
	
	if (temperature < 0.5f)
		return BiomeType::TAIGA;
	
	if (temperature < 0.97f)
	{
		if (downfall < 0.35f)
			return BiomeType::SHRUBLAND;
		else
			return BiomeType::FOREST;
	}
	
	if (downfall < 0.45f)
		return BiomeType::PLAINS;
	else if (downfall < 0.9f)
		return BiomeType::SEASONAL_FOREST;
	else
		return BiomeType::RAINFOREST;
}

// Alpha: Cold biomes are tundra, taiga, iceDesert (those that call func_4122_b() in MobSpawnerBase)
bool BiomeSource::isCold(BiomeType biome)
{
	return biome == BiomeType::TUNDRA || 
	       biome == BiomeType::TAIGA || 
	       biome == BiomeType::ICE_DESERT;
}

// Alpha: Desert biomes are desert, savanna, plains, iceDesert (MobSpawnerDesert instances)
bool BiomeSource::isDesert(BiomeType biome)
{
	return biome == BiomeType::DESERT || 
	       biome == BiomeType::SAVANNA || 
	       biome == BiomeType::PLAINS || 
	       biome == BiomeType::ICE_DESERT;
}

// Get biome at specific coordinates using temperature/downfall from arrays
// Note: This assumes getBiomeBlock() has been called for the chunk containing (x, z)
BiomeType BiomeSource::getBiomeAt(int_t x, int_t z)
{
	// Convert world coordinates to local chunk coordinates (0-15)
	int_t localX = x & 15;
	int_t localZ = z & 15;
	int_t index = localX + localZ * 16;
	
	if (index < 0 || index >= 256)
		index = 0;
	
	float temp = static_cast<float>(temperatures[index]);
	float down = static_cast<float>(downfalls[index]);
	
	return getBiome(temp, down);
}

// Beta: BiomeSource.getTemperatureBlock() - returns temperature array for snow generation
std::array<double, 16 * 16> &BiomeSource::getTemperatureBlock(int_t x, int_t z, int_t xd, int_t zd)
{
	// Beta: getTemperatureBlock() - similar to getBiomeBlock() but only processes temperatures
	temperatureMap.getRegion(temperatures.data(), x, z, xd, xd, 0.025, 0.025, 0.25);
	noiseMap.getRegion(noises.data(), x, z, xd, xd, 0.25, 0.25, 0.5882352941176471);
	
	int_t i2 = 0;
	for (int_t xi = 0; xi < xd; xi++)
	{
		for (int_t zi = 0; zi < zd; zi++)
		{
			double nv = noises[i2] * 1.1 + 0.5;
			double d2 = 0.01;
			double d3 = 1.0 - d2;
			double temperature = (temperatures[i2] * 0.15 + 0.7) * d3 + nv * d2;
			temperature = 1.0 - (1.0 - temperature) * (1.0 - temperature);
			if (temperature < 0.0) temperature = 0.0;
			if (temperature > 1.0) temperature = 1.0;
			temperatures[i2] = temperature;
			i2++;
		}
	}
	
	return temperatures;
}