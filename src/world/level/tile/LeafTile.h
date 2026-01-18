#pragma once

#include "world/level/tile/TransparentTile.h"

class LeafTile : public TransparentTile
{
public:
	static constexpr int_t REQUIRED_WOOD_RANGE = 4;

	static constexpr int_t UPDATE_LEAF_BIT = 4;  // Alpha: bit 2 = 4, used for leaf decay checking

	// Alpha 1.2.6: BlockLeaves has no type variants (no birch/spruce) - only oak leaves exist
	// EVERGREEN_LEAF and BIRCH_LEAF are Beta 1.2 features, not Alpha
	// Metadata is only used for decay distance calculation, not for leaf type

private:
	int_t oTex = 0;
	static int_t *checkBuffer;  // Beta: LeafTile.checkBuffer - array for decay checking (LeafTile.java:18) - static since LeafTile is singleton

public:
	LeafTile(int_t id, int_t tex);

	int_t getColor(LevelSource &level, int_t x, int_t y, int_t z) override;

	void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;

private:
	void die(Level &level, int_t x, int_t y, int_t z);

public:
	int_t getResourceCount(Random &random) override;
	int_t getResource(int_t data, Random &random) override;

	bool isSolidRender() override;

	int_t getTexture(Facing face, int_t data) override;

	void setFancy(bool fancy);

	void stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
};
