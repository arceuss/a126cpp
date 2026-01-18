#pragma once

#include "world/level/tile/TransparentTile.h"

// Beta 1.2: ReedTile.java - ID 83, texture 73
// ReedTile uses Material.plant (DecorationMaterial)
class ReedTile : public TransparentTile
{
public:
	ReedTile(int_t id, int_t tex);
	
	// Beta: ReedTile.tick() - grows upward (ReedTile.java:19-37)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: ReedTile.mayPlace() - checks for water adjacent or reed below (ReedTile.java:40-53)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: ReedTile.neighborChanged() calls checkAlive() (ReedTile.java:56-58)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: ReedTile.getResource() returns Item.reeds.id (ReedTile.java:78-80)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: ReedTile.getRenderShape() returns 1 (ReedTile.java:97-99)
	virtual Shape getRenderShape() override;
	
	// Beta: ReedTile.isCubeShaped() returns false (ReedTile.java:92-94)
	virtual bool isCubeShaped() override;
	
	// Beta: ReedTile.isSolidRender() returns false (ReedTile.java:87-89)
	virtual bool isSolidRender() override;
	
	// Beta: ReedTile.blocksLight() returns false (ReedTile.java:82-84)
	bool blocksLight() { return false; }
	
	// Beta: ReedTile.getAABB() returns null (ReedTile.java:73-75)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
protected:
	// Beta: ReedTile.checkAlive() - checks if can survive, removes if not (ReedTile.java:60-65)
	void checkAlive(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: ReedTile.canSurvive() delegates to mayPlace() (ReedTile.java:68-70)
	bool canSurvive(Level &level, int_t x, int_t y, int_t z);
};
