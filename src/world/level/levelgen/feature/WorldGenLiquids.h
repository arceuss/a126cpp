#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenLiquids (water/lava springs)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenLiquids.java
class WorldGenLiquids : public WorldGenerator
{
private:
	int_t liquidBlockId;  // Block ID for the liquid (waterStill or lavaStill)

public:
	WorldGenLiquids(int_t blockId);
	
	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
