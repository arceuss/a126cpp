#pragma once

#include "world/level/tile/Tile.h"

class TreeTile : public Tile
{
public:
	// Alpha 1.2.6: BlockLog has no metadata variants - only oak logs exist
	// DARK_TRUNK and BIRCH_TRUNK are Beta 1.2 features, not Alpha

	TreeTile(int_t id);
	
	int_t getResourceCount(Random &random) override;
	int_t getResource(int_t data, Random &random) override;

	void onRemove(Level &level, int_t x, int_t y, int_t z) override;

	int_t getTexture(Facing face, int_t data) override;

	int_t getSpawnResourcesAuxValue(int_t data) override;
};
