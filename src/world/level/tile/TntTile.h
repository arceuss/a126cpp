#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 TntTile.java
// TNT block that explodes when triggered by redstone or destroyed
class TntTile : public Tile
{
public:
	TntTile(int_t id, int_t texture);
	
	int_t getTexture(Facing face) override;
	void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	int_t getResourceCount(Random &random) override;
	void destroy(Level &level, int_t x, int_t y, int_t z, int_t data) override;
	void wasExploded(Level &level, int_t x, int_t y, int_t z) override;
};
