#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 Sponge.java
// Sponge block that absorbs water in 2-block radius
class SpongeTile : public Tile
{
public:
	static const int_t RANGE = 2;

	SpongeTile(int_t id);
	
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
};
