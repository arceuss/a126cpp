#pragma once

#include "world/level/tile/TransparentTile.h"

// Alpha: BlockIce.java
class IceTile : public TransparentTile
{
public:
	IceTile(int_t id, int_t tex);
	
	// Alpha: BlockIce.quantityDropped() returns 0 (BlockIce.java:28-30)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Alpha: BlockIce.updateTick() - melts when light > 11 - lightOpacity (BlockIce.java:32-37)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Alpha: BlockIce.onBlockRemoval() - converts to water if block below is solid/liquid (BlockIce.java:20-26)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	
	// Alpha: BlockIce.shouldSideBeRendered() - reverses face for transparent rendering (BlockIce.java:16-18)
	virtual bool shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Alpha: Ice renders in translucent pass (pass 1) like water
	virtual int_t getRenderLayer() override;
};
