#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: FenceTile.java - ID 85, texture varies
// Alpha 1.2.6: BlockFence - ID 85
class FenceTile : public Tile
{
public:
	FenceTile(int_t id, int_t tex);

	// Beta: FenceTile.addAABBs() - adds fence AABB (1.5 blocks high) (FenceTile.java:13-15)
	virtual void addAABBs(Level &level, int_t x, int_t y, int_t z, AABB &bb, std::vector<AABB *> &aabbList) override;
	
	// Beta: FenceTile.getAABB() - returns fence AABB (1.5 blocks high) for collision
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: FenceTile.mayPlace() - checks if can be placed on solid block (FenceTile.java:18-24)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: FenceTile.blocksLight() returns false (FenceTile.java:27-29)
	bool blocksLight() { return false; }

	// Beta: FenceTile.isSolidRender() returns false (FenceTile.java:31-33)
	virtual bool isSolidRender() override;

	// Beta: FenceTile.isCubeShaped() returns false (FenceTile.java:36-38)
	virtual bool isCubeShaped() override;

	// Beta: FenceTile.getRenderShape() returns 11 (SHAPE_FENCE) (FenceTile.java:41-43)
	virtual Shape getRenderShape() override;
};
