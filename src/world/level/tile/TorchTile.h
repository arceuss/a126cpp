#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: TorchTile.java - ID 50, texture 80, Material.decoration
// Alpha 1.2.6: BlockTorch - ID 50, texture 80
class TorchTile : public Tile
{
public:
	TorchTile(int_t id, int_t tex);
	
	// Beta: TorchTile.getAABB() returns null (no collision) (TorchTile.java:17-19)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: TorchTile.blocksLight() returns false (TorchTile.java:21-23)
	bool blocksLight() { return false; }
	
	// Beta: TorchTile.isSolidRender() returns false (TorchTile.java:26-28)
	virtual bool isSolidRender() override;
	
	// Beta: TorchTile.isCubeShaped() returns false (TorchTile.java:31-33)
	virtual bool isCubeShaped() override;
	
	// Beta: TorchTile.getRenderShape() returns 2 (TorchTile.java:36-38)
	virtual Shape getRenderShape() override;
	
	// Beta: TorchTile.mayPlace() - checks if can be placed on solid block (TorchTile.java:41-51)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: TorchTile.setPlacedOnFace() - sets orientation based on face (TorchTile.java:54-77)
	virtual void setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: TorchTile.tick() - calls onPlace if data is 0 (TorchTile.java:80-85)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: TorchTile.onPlace() - sets orientation based on adjacent solid blocks (TorchTile.java:88-102)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: TorchTile.neighborChanged() - checks if can survive, drops if not (TorchTile.java:105-134)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: TorchTile.clip() - custom hitbox based on orientation (TorchTile.java:147-164)
	virtual HitResult clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to) override;
	
	// Beta: TorchTile.animateTick() - adds smoke and flame particles (TorchTile.java:167-190)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
private:
	// Beta: TorchTile.checkCanSurvive() - checks if can survive, drops if not (TorchTile.java:136-144)
	bool checkCanSurvive(Level &level, int_t x, int_t y, int_t z);
};
