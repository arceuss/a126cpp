#include "world/level/biome/FixedBiomeSource.h"

#include <algorithm>

FixedBiomeSource::FixedBiomeSource(Level &level, BiomeType biome, double temperature, double downfall) :
	BiomeSource(level), biome(biome), temperature(temperature), downfall(downfall)
{

}

// Alpha: WorldChunkManagerHell.func_4072_b() (WorldChunkManagerHell.java:31-33)
double FixedBiomeSource::getTemperature(int_t x, int_t z)
{
	temperatures[0] = temperature;
	return temperature;
}

// Alpha: WorldChunkManagerHell.loadBlockGeneratorData() fills the whole region
// with the fixed biome, downfall and temperature (WorldChunkManagerHell.java:48-58)
void FixedBiomeSource::getBiomeBlock(int_t x, int_t z, int_t xd, int_t zd)
{
	size_t count = std::min(static_cast<size_t>(xd * zd), temperatures.size());
	std::fill_n(temperatures.begin(), count, temperature);
	std::fill_n(downfalls.begin(), count, downfall);
}

// Alpha: WorldChunkManagerHell.getTemperatures() (WorldChunkManagerHell.java:40-46)
std::array<double, 16 * 16> &FixedBiomeSource::getTemperatureBlock(int_t x, int_t z, int_t xd, int_t zd)
{
	size_t count = std::min(static_cast<size_t>(xd * zd), temperatures.size());
	std::fill_n(temperatures.begin(), count, temperature);
	return temperatures;
}

// Alpha: WorldChunkManagerHell.func_4073_a() answers the same biome everywhere
// (WorldChunkManagerHell.java:27-29)
BiomeType FixedBiomeSource::getBiomeAt(int_t x, int_t z)
{
	return biome;
}
