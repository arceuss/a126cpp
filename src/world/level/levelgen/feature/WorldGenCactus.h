#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenCactus
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenCactus.java
class WorldGenCactus : public WorldGenerator
{
public:
	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
