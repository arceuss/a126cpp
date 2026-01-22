#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenLakes (water/lava lakes)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenLakes.java
class WorldGenLakes : public WorldGenerator
{
private:
	int_t liquidBlockId;  // Block ID for the liquid (waterMoving or lavaMoving)

public:
	WorldGenLakes(int_t blockId);
	
	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
