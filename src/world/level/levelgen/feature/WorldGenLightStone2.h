#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenLightStone2 (glowstone cluster hanging from a netherrack ceiling)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenLightStone2.java
class WorldGenLightStone2 : public WorldGenerator
{
public:
	WorldGenLightStone2();

	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
