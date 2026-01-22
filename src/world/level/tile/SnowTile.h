#pragma once

#include "world/level/tile/TransparentTile.h"

// Beta 1.2: TopSnowTile.java - ID 78, texture 66
// TopSnowTile uses Material.topSnow (DecorationMaterial)
class SnowTile : public TransparentTile
{
public:
	SnowTile(int_t id, int_t texture);
	
	// Alpha: BlockSnow.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 2.0F / 16.0F, 1.0F)
	// Beta: TopSnowTile.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.125F, 1.0F) - 0.125F = 2.0F / 16.0F
	
	// Alpha: BlockSnow.isOpaqueCube() returns false
	virtual bool isSolidRender() override { return false; }
	
	// Alpha: BlockSnow.renderAsNormalBlock() returns false
	virtual bool isCubeShaped() override { return false; }
	
	// Alpha: BlockSnow.getCollisionBoundingBoxFromPool() returns null
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Alpha: BlockSnow.onNeighborBlockChange() calls func_314_h()
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Alpha: BlockSnow.idDropped() returns Item.snowball.shiftedIndex
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Alpha: BlockSnow.quantityDropped() returns 0
	virtual int_t getResourceCount(Random &random) override;
	
	// Alpha: BlockSnow.updateTick() - melts if light > 11
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: TopSnowTile.shouldRenderFace() - special handling for snow material
	virtual bool shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: TopSnowTile.playerDestroy() - manually spawns ItemEntity (TopSnowTile.java:61-71)
	virtual void playerDestroy(Level &level, int_t x, int_t y, int_t z, int_t data) override;
	
	// Beta: TopSnowTile.mayPlace() - checks if can be placed (TopSnowTile.java:40-43)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: TopSnowTile.blocksLight() returns false (TopSnowTile.java:25-27)
	bool blocksLight() { return false; }

private:
	// Beta: TopSnowTile.checkCanSurvive() - checks if can survive, removes if not (TopSnowTile.java:50-58)
	bool checkCanSurvive(Level &level, int_t x, int_t y, int_t z);
};
