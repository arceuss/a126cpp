#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 TntTile.java
// TNT block that explodes when triggered by redstone or destroyed
class TntTile : public Tile
{
public:
	TntTile(int_t id, int_t texture);
	
	virtual int_t getTexture(Facing face) override;
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	virtual int_t getResourceCount(Random &random) override;
	virtual void destroy(Level &level, int_t x, int_t y, int_t z, int_t data) override;
	// TODO: wasExploded() method when explosions are implemented
};
