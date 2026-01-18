#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: HellStoneTile.java - ID 87, texture 88
// Alpha: Block.bloodStone (Block.java:108)
// HellStoneTile uses Material.stone
class HellStoneTile : public Tile
{
public:
	HellStoneTile(int_t id, int_t texture);
};
