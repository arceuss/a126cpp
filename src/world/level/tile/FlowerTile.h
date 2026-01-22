#pragma once

#include "world/level/tile/TransparentTile.h"

// Alpha 1.2.6 BlockFlower (dandelion/rose)
// Reference: apclient/minecraft/src/net/minecraft/src/BlockFlower.java
class FlowerTile : public TransparentTile
{
public:
	FlowerTile(int_t id, int_t tex);

	// Alpha: BlockFlower.canPlaceBlockAt() checks canThisPlantGrowOnThisBlockID
	virtual bool mayPick() override;
	
	// Alpha: BlockFlower.canBlockStay() checks light level and block below
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Alpha: BlockFlower.getRenderType() returns 1 (cross-texture rendering)
	virtual Shape getRenderShape() override;
	
	// Alpha: BlockFlower.renderAsNormalBlock() returns false
	virtual bool isCubeShaped() override;
	
	// Alpha: BlockFlower.getCollisionBoundingBoxFromPool() returns null (no collision)
	// Beta: getTileAABB() uses shape (from Tile base class) for hitbox rendering
	// Only getAABB() returns null (no collision)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
public:
	// Alpha: BlockFlower.canPlaceBlockAt() checks canThisPlantGrowOnThisBlockID
	virtual bool canPlaceBlockAt(Level &level, int_t x, int_t y, int_t z);
	
	// Alpha: BlockFlower.onNeighborBlockChange() calls func_268_h (validation)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Override clip to ensure hit detection works (plants have no collision but should still be selectable)
	// The default clip() uses shape bounds which should work, but we need to ensure it handles cross-texture blocks correctly
	virtual HitResult clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to) override;
	
protected:
	// Alpha: BlockFlower.canThisPlantGrowOnThisBlockID() - returns true for grass, dirt, tilledField
	virtual bool canThisPlantGrowOnThisBlockID(int_t blockId);
	
	// Alpha: BlockFlower.canBlockStay() - checks light >= 8 or canSeeSky, and valid block below
	bool canBlockStay(Level &level, int_t x, int_t y, int_t z);
};
