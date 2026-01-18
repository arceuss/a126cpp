#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2 Tile.clay (ID 82, texture 72)
// Alpha: Block.blockClay (Block.java:104)
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/tile/ClayTile.java
class ClayTile : public Tile
{
public:
	ClayTile(int_t id, int_t texture);
	
	// Beta: ClayTile.getResource() returns Item.clay.id
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: ClayTile.getResourceCount() returns 4
	virtual int_t getResourceCount(Random &random) override;
};
