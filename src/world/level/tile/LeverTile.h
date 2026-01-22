#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: LeverTile.java - ID 69, texture varies
// Alpha 1.2.6: Lever blocks exist
class LeverTile : public Tile
{
public:
	LeverTile(int_t id, int_t tex);

	// Beta: LeverTile.getAABB() - returns null (no collision) (LeverTile.java:15-17)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: LeverTile.blocksLight() returns false (LeverTile.java:19-21)
	bool blocksLight() { return false; }

	// Beta: LeverTile.isSolidRender() returns false (LeverTile.java:24-26)
	virtual bool isSolidRender() override;

	// Beta: LeverTile.isCubeShaped() returns false (LeverTile.java:29-31)
	virtual bool isCubeShaped() override;

	// Beta: LeverTile.getRenderShape() returns 12 (SHAPE_LEVER) (LeverTile.java:34-36)
	virtual Shape getRenderShape() override;

	// Beta: LeverTile.mayPlace() - checks if can be placed on solid block (LeverTile.java:39-49)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: LeverTile.setPlacedOnFace() - sets orientation based on face (LeverTile.java:52-77)
	virtual void setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face) override;

	// Beta: LeverTile.onPlace() - sets orientation based on adjacent solid blocks (LeverTile.java:80-94)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: LeverTile.neighborChanged() - checks if lever can survive (LeverTile.java:97-126)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;

	// Beta: LeverTile.updateShape() - sets shape based on data (LeverTile.java:139-154)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;

	// Beta: LeverTile.attack() - calls use() (LeverTile.java:157-159)
	virtual void attack(Level &level, int_t x, int_t y, int_t z, Player &player) override;

	// Beta: LeverTile.use() - toggles lever state (LeverTile.java:162-187)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;

	// Beta: LeverTile.onRemove() - updates neighbors when removed (LeverTile.java:190-209)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: LeverTile.getSignal() - returns signal state (LeverTile.java:212-214)
	virtual bool getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing) override;

	// Beta: LeverTile.getDirectSignal() - returns direct signal (LeverTile.java:217-233)
	virtual bool getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing) override;

	// Beta: LeverTile.isSignalSource() returns true (LeverTile.java:236-238)
	virtual bool isSignalSource() override;

private:
	// Beta: LeverTile.checkCanSurvive() - checks if lever can survive (LeverTile.java:128-136)
	bool checkCanSurvive(Level &level, int_t x, int_t y, int_t z);
};
