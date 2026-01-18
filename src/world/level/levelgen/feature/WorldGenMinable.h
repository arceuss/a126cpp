#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenMinable (ore veins)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenMinable.java
class WorldGenMinable : public WorldGenerator
{
private:
	int_t minableBlockId;  // Block ID for the ore to place
	int_t numberOfBlocks;  // Vein size

public:
	WorldGenMinable(int_t blockId, int_t veinSize);
	
	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
