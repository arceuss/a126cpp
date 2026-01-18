#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 Tile.unbreakable (Tile.java:76-80)
// unbreakable = new Tile(7, 17, Material.stone).setDestroyTime(-1.0F).setExplodeable(6000000.0F).setSoundType(SOUND_STONE)
class UnbreakableTile : public Tile
{
public:
	UnbreakableTile(int_t id, int_t texture);
};
