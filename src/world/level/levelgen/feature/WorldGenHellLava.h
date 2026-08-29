#pragma once

#include "world/level/levelgen/feature/WorldGenerator.h"
#include "java/Type.h"

// Alpha 1.2.6 WorldGenHellLava (nether lava springs)
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenHellLava.java
class WorldGenHellLava : public WorldGenerator
{
private:
	int_t liquidTileId;  // Alpha: field_4158_a (WorldGenHellLava.java:13)

public:
	WorldGenHellLava(int_t tileId);

	bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
