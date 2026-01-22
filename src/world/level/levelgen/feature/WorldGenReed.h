#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenReed (sugar cane)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenReed.java
class WorldGenReed : public WorldGenerator
{
public:
	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
