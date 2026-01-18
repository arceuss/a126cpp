#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 Tile.wood (Tile.java:74)
// wood = new Tile(5, 4, Material.wood).setDestroyTime(2.0F).setExplodeable(5.0F).setSoundType(SOUND_WOOD)
class WoodTile : public Tile
{
public:
	WoodTile(int_t id, int_t texture);
};
