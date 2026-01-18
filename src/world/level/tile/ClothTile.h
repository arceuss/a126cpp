#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Alpha 1.2.6 Block.cloth (Block.java:57)
// Simple cloth block - no metadata/colored wool support in Alpha
class ClothTile : public Tile
{
public:
	ClothTile();
};
