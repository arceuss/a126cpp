#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 Tile.redBrick (Tile.java:154-158)
// redBrick = new Tile(45, 7, Material.stone).setDestroyTime(2.0F).setExplodeable(10.0F).setSoundType(SOUND_STONE)
class RedBrickTile : public Tile
{
public:
	RedBrickTile(int_t id, int_t texture);
};
