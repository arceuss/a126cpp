#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2 Tile.mossStone (ID 48, texture 36)
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/tile/Tile.java:161-165
class MossyCobblestoneTile : public Tile
{
public:
	MossyCobblestoneTile(int_t id, int_t texture);
};
