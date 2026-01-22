#pragma once

#include "world/level/tile/Tile.h"
#include "world/level/TilePos.h"
#include <unordered_set>
#include <vector>

// Beta 1.2: RedStoneDustTile.java - ID 55, texture 55, Material.decoration
// Alpha 1.2.6: BlockRedstoneWire - ID 55, texture 55
class RedStoneDustTile : public Tile
{
private:
	bool shouldSignal;
	std::unordered_set<TilePos> toUpdate;

	// Beta: RedStoneDustTile.updatePowerStrength() - updates power strength (RedStoneDustTile.java:53-62)
	void updatePowerStrength(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: RedStoneDustTile.updatePowerStrength() - recursive power update (RedStoneDustTile.java:64-174)
	void updatePowerStrength(Level &level, int_t x, int_t y, int_t z, int_t fromX, int_t fromY, int_t fromZ);
	
	// Beta: RedStoneDustTile.checkCornerChangeAt() - checks corner changes (RedStoneDustTile.java:176-186)
	void checkCornerChangeAt(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: RedStoneDustTile.checkTarget() - checks target power (RedStoneDustTile.java:262-269)
	int_t checkTarget(Level &level, int_t x, int_t y, int_t z, int_t currentPower);

public:
	RedStoneDustTile(int_t id, int_t tex);
	
	// Beta: RedStoneDustTile.getTexture() - returns texture based on data (RedStoneDustTile.java:23-26)
	virtual int_t getTexture(Facing face, int_t data) override;
	
	// Beta: RedStoneDustTile.getAABB() - returns null (no collision) (RedStoneDustTile.java:28-31)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: RedStoneDustTile.isSolidRender() - returns false (RedStoneDustTile.java:33-36)
	virtual bool isSolidRender() override;
	
	// Beta: RedStoneDustTile.isCubeShaped() - returns false (RedStoneDustTile.java:38-41)
	virtual bool isCubeShaped() override;
	
	// Beta: RedStoneDustTile.getRenderShape() - returns 5 (SHAPE_RED_DUST) (RedStoneDustTile.java:43-46)
	virtual Shape getRenderShape() override;
	
	// Beta: RedStoneDustTile.mayPlace() - checks if can be placed on solid block (RedStoneDustTile.java:48-51)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: RedStoneDustTile.onPlace() - updates power strength (RedStoneDustTile.java:188-223)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: RedStoneDustTile.onRemove() - updates power strength (RedStoneDustTile.java:225-260)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: RedStoneDustTile.neighborChanged() - updates power strength (RedStoneDustTile.java:271-285)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: RedStoneDustTile.getResource() - returns redstone item (RedStoneDustTile.java:287-290)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: RedStoneDustTile.getDirectSignal() - returns direct signal (RedStoneDustTile.java:292-295)
	virtual bool getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing) override;
	
	// Beta: RedStoneDustTile.getSignal() - returns signal strength (RedStoneDustTile.java:297-342)
	virtual bool getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing) override;
	
	// Beta: RedStoneDustTile.isSignalSource() - returns shouldSignal (RedStoneDustTile.java:344-347)
	virtual bool isSignalSource() override;
	
	// Beta: RedStoneDustTile.animateTick() - adds reddust particles (RedStoneDustTile.java:349-357)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: RedStoneDustTile.shouldConnectTo() - static helper (RedStoneDustTile.java:359-366)
	static bool shouldConnectTo(LevelSource &level, int_t x, int_t y, int_t z);
};
