#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: StoneSlabTile.java - ID 43 (double) or 44 (half), texture 6, Material.stone
// Alpha 1.2.6: BlockStep - ID 43 (double) or 44 (half), texture 6
class StoneSlabTile : public Tile
{
private:
	bool fullSize;  // Beta: fullSize (StoneSlabTile.java:9) - true for double slab, false for half

public:
	StoneSlabTile(int_t id, bool fullSize);
	
	// Beta: StoneSlabTile.getTexture() - different textures for top/bottom vs sides (StoneSlabTile.java:22-24)
	virtual int_t getTexture(Facing face, int_t data) override;
	
	// Beta: StoneSlabTile.isSolidRender() - returns fullSize (StoneSlabTile.java:27-29)
	virtual bool isSolidRender() override;
	
	// Beta: StoneSlabTile.isCubeShaped() - returns fullSize (StoneSlabTile.java:57-59)
	virtual bool isCubeShaped() override;
	
	// Beta: StoneSlabTile.neighborChanged() - empty for half slab (StoneSlabTile.java:32-36)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: StoneSlabTile.onPlace() - combines with half slab below to make double (StoneSlabTile.java:39-49)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: StoneSlabTile.getResource() - double slab drops half slab (StoneSlabTile.java:52-54)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: StoneSlabTile.shouldRenderFace() - special face rendering logic (StoneSlabTile.java:62-74)
	virtual bool shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
};
