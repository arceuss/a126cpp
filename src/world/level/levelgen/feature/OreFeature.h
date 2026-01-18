#pragma once

#include "world/level/levelgen/feature/Feature.h"
#include "java/Type.h"

class Level;
class Random;

// Beta 1.2: OreFeature.java - generates ore veins in the world
class OreFeature : public Feature
{
private:
	int_t tile;   // Beta: OreFeature.tile (OreFeature.java:9)
	int_t count; // Beta: OreFeature.count (OreFeature.java:10)

public:
	// Beta: OreFeature(int var1, int var2) (OreFeature.java:12-15)
	OreFeature(int_t tileId, int_t veinCount);

	// Beta: OreFeature.place() (OreFeature.java:18-60)
	bool place(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
