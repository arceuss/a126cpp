#pragma once

#include "world/level/tile/FlowerTile.h"

// Alpha 1.2.6 BlockMushroom (brown/red)
// Reference: apclient/minecraft/src/net/minecraft/src/BlockMushroom.java
// BlockMushroom extends BlockFlower, so it inherits getRenderType() = 1 (cross-texture) and null collision box
class MushroomTile : public FlowerTile
{
public:
	MushroomTile(int_t id, int_t tex);
	
	// Alpha: BlockMushroom overrides canBlockStay() - requires light <= 13 (BlockMushroom.java:14-15)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
protected:
	// Alpha: BlockMushroom.canThisPlantGrowOnThisBlockID() returns true for any opaque block (BlockMushroom.java:10-11)
	bool canThisPlantGrowOnThisBlockID(int_t blockId);  // Not virtual in base, so no override
	
	// Alpha: BlockMushroom.canBlockStay() - light <= 13 and opaque block below (BlockMushroom.java:14-15)
	bool canBlockStay(Level &level, int_t x, int_t y, int_t z);  // Not virtual in base, so no override
};
