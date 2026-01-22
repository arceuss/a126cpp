#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: FarmTile.java - ID 60, texture 87, Material.dirt
// Alpha 1.2.6: BlockFarmland/tilledField - ID 60
class FarmTile : public Tile
{
public:
	FarmTile(int_t id);
	
	// Beta: FarmTile overrides
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	virtual bool isSolidRender() override;
	virtual bool isCubeShaped() override;
	virtual int_t getTexture(Facing face, int_t data) override;
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	virtual void stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	virtual int_t getResource(int_t data, Random &random) override;
	
private:
	// Beta: FarmTile helper methods
	bool isUnderCrops(Level &level, int_t x, int_t y, int_t z);
	bool isNearWater(Level &level, int_t x, int_t y, int_t z);
};
