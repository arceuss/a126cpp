#pragma once

#include "world/level/levelgen/feature/Feature.h"
#include "java/Type.h"

class Level;
class Random;

// Beta 1.2: PumpkinFeature.java - generates pumpkins in the world
class PumpkinFeature : public Feature
{
public:
	// Beta: PumpkinFeature.place() (PumpkinFeature.java:9-20)
	bool place(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
