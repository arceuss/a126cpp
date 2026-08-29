#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenFire (fire patches on netherrack)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenFire.java
class WorldGenFire : public WorldGenerator
{
public:
	WorldGenFire();

	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
