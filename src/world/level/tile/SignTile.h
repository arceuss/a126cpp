#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: SignTile.java - ID 63 (sign post), ID 68 (wall sign)
// Alpha 1.2.6: Sign blocks exist
// Note: SignTile extends EntityTile in Java, but in C++ we just override newTileEntity()
class SignTile : public Tile
{
private:
	bool onGround;  // Beta: private boolean onGround (SignTile.java:13) - true for sign post, false for wall sign

public:
	SignTile(int_t id, bool onGround);
	
	// Beta: SignTile.getAABB() - returns null (no collision) (SignTile.java:26-28)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: SignTile.getTileAABB() - calls updateShape() first (SignTile.java:30-34)
	virtual AABB *getTileAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: SignTile.updateShape() - adjusts shape based on data for wall signs (SignTile.java:36-62)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;
	
	// Beta: SignTile.getRenderShape() - returns SHAPE_INVISIBLE (-1) (SignTile.java:64-67)
	virtual Shape getRenderShape() override;
	
	// Beta: SignTile.isCubeShaped() - returns false (SignTile.java:69-72)
	virtual bool isCubeShaped() override;
	
	// Beta: SignTile.isSolidRender() - returns false (SignTile.java:74-77)
	virtual bool isSolidRender() override;
	
	// Beta: SignTile.newTileEntity() - returns SignTileEntity (SignTile.java:79-86)
	virtual std::shared_ptr<TileEntity> newTileEntity() override;
	
	// Beta: SignTile.getResource() - returns Item.sign.id (SignTile.java:88-91)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: SignTile.neighborChanged() - checks if sign can survive (SignTile.java:93-126)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: EntityTile.onPlace() - creates TileEntity (EntityTile.java:19-22)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: EntityTile.onRemove() - removes TileEntity (EntityTile.java:24-28)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
};
