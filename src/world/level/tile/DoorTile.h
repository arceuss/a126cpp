#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: DoorTile.java - ID 64 (wood), ID 71 (iron)
// Alpha 1.2.6: Door blocks exist
class DoorTile : public Tile
{
public:
	DoorTile(int_t id, const Material &material);

	// Beta: DoorTile.getTexture() - returns texture based on face and data (DoorTile.java:27-45)
	virtual int_t getTexture(Facing face, int_t data) override;

	// Beta: DoorTile.blocksLight() returns false (DoorTile.java:47-49)
	bool blocksLight() { return false; }

	// Beta: DoorTile.isSolidRender() returns false (DoorTile.java:52-54)
	virtual bool isSolidRender() override;

	// Beta: DoorTile.isCubeShaped() returns false (DoorTile.java:57-59)
	virtual bool isCubeShaped() override;

	// Beta: DoorTile.getRenderShape() returns 7 (SHAPE_DOOR) (DoorTile.java:62-64)
	virtual Shape getRenderShape() override;

	// Beta: DoorTile.getTileAABB() - updates shape and returns AABB (DoorTile.java:67-70)
	virtual AABB *getTileAABB(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: DoorTile.getAABB() - updates shape and returns AABB (DoorTile.java:73-76)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: DoorTile.updateShape() - sets shape based on direction (DoorTile.java:79-81)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;

	// Beta: DoorTile.attack() - calls use() (DoorTile.java:104-106)
	virtual void attack(Level &level, int_t x, int_t y, int_t z, Player &player) override;

	// Beta: DoorTile.use() - toggles door state (DoorTile.java:109-136)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;

	// Beta: DoorTile.setOpen() - sets door open/closed state (DoorTile.java:138-160)
	void setOpen(Level &level, int_t x, int_t y, int_t z, bool open);

	// Beta: DoorTile.neighborChanged() - handles door updates (DoorTile.java:163-195)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;

	// Beta: DoorTile.getResource() - returns door item (DoorTile.java:198-204)
	virtual int_t getResource(int_t data, Random &random) override;

	// Beta: DoorTile.clip() - updates shape and clips (DoorTile.java:207-210)
	virtual HitResult clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to) override;

	// Beta: DoorTile.mayPlace() - checks if door can be placed (DoorTile.java:217-221)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;

private:
	// Beta: DoorTile.setShape() - sets shape based on direction (DoorTile.java:83-101)
	void setShape(int_t dir);

	// Beta: DoorTile.getDir() - gets door direction from data (DoorTile.java:212-214)
	int_t getDir(int_t data);
};
