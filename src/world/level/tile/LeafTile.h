#pragma once

#include "world/level/tile/TransparentTile.h"

class LeafTile : public TransparentTile
{
public:
	// Alpha 1.2.6: BlockLeaves has no type variants (no birch/spruce) - only oak leaves exist
	// EVERGREEN_LEAF and BIRCH_LEAF are Beta 1.2 features, not Alpha

private:
	int_t oTex = 0;

public:
	LeafTile(int_t id, int_t tex);

	int_t getColor(LevelSource &level, int_t x, int_t y, int_t z) override;

public:
	int_t getResourceCount(Random &random) override;
	int_t getResource(int_t data, Random &random) override;

	bool isSolidRender() override;

	int_t getTexture(Facing face, int_t data) override;

	void setFancy(bool fancy);

	void stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
};
