#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 MetalTile.java
// Simple tile that uses Material.metal and returns fixed texture
class MetalTile : public Tile
{
public:
	MetalTile(int_t id, int_t texture);
	
	virtual int_t getTexture(Facing face) override;
};
