#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: LadderTile.java - ID 65
// Alpha 1.2.6: Ladder blocks exist but may differ in behavior
class LadderTile : public Tile
{
public:
	LadderTile(int_t id, int_t tex);
	
	// Beta: LadderTile.getAABB() - collision bounding box based on facing (LadderTile.java:17-30)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LadderTile.getTileAABB() - collision bounding box for tile entities (LadderTile.java:36-57)
	virtual AABB *getTileAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LadderTile.getRenderShape() - returns SHAPE_LADDER (LadderTile.java:32-34)
	virtual Shape getRenderShape() override;
	
	// Beta: LadderTile.isSolidRender() - returns false (non-solid) (LadderTile.java:36-38)
	virtual bool isSolidRender() override;
	
	// Beta: LadderTile.mayPlace() - checks if can attach to adjacent solid block (LadderTile.java:40-50)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LadderTile.onPlace() - checks neighbor blocks and sets data (LadderTile.java:52-68)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LadderTile.setPlacedOnFace() - sets orientation based on face (LadderTile.java:90-109)
	virtual void setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: LadderTile.neighborChanged() - removes if attached block is gone (LadderTile.java:112-137)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: LadderTile.getResourceCount() - returns 1 (LadderTile.java:140-142)
	virtual int_t getResourceCount(Random &random) override;
};
