#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenLightStone1 (glowstone cluster hanging from a netherrack ceiling)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenLightStone1.java
class WorldGenLightStone1 : public WorldGenerator
{
public:
	WorldGenLightStone1();

	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
