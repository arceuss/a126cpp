#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Alpha 1.2.6 Block.cobblestone (Block.java:26)
// cobblestone = (new Block(4, 16, Material.rock)).setHardness(2.0F).setResistance(10.0F).setStepSound(soundStoneFootstep)
class CobblestoneTile : public Tile
{
public:
	CobblestoneTile(int_t id, int_t texture);
};
