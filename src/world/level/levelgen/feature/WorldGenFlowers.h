#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenFlowers
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenFlowers.java
class WorldGenFlowers : public WorldGenerator
{
private:
	int_t plantBlockId;  // Block ID for the flower/plant to place

public:
	WorldGenFlowers(int_t blockId);
	
	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
