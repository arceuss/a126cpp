#pragma once

#include "world/level/tile/FlowerTile.h"

// Beta 1.2: Sapling.java - extends Bush
// Alpha 1.2.6: Saplings exist (no new tree types like birch/evergreen)
class SaplingTile : public FlowerTile
{
public:
	SaplingTile(int_t id, int_t tex);
	
	// Beta: Sapling.tick() - grows tree if conditions are met (Sapling.java:17-27)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: Bush.mayPlace() - checks if can be placed on block below (Bush.java:18-20)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: Bush.neighborChanged() - checks if can survive (Bush.java:27-30)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
protected:
	// Beta: Bush.mayPlaceOn() - checks if block below is valid (Bush.java:22-24)
	bool mayPlaceOn(int_t blockId);
	
	// Beta: Bush.canSurvive() - checks if can survive (Bush.java:45-47)
	bool canSurvive(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: Bush.checkAlive() - checks if can survive, drops if not (Bush.java:37-42)
	void checkAlive(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: Sapling.growTree() - generates tree (Sapling.java:29-39)
	// Note: Tree generation not implemented yet, so this is a placeholder
	void growTree(Level &level, int_t x, int_t y, int_t z, Random &random);
};
