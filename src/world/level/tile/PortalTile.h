#pragma once

#include "world/level/tile/TransparentTile.h"

// Beta 1.2: PortalTile.java - ID 90, texture 91
// Alpha: BlockPortal (Block.java:110)
// PortalTile extends HalfTransparentTile (TransparentTile in C++), uses Material.portal
// Note: FX/animation is already ported, so we focus on core logic
class PortalTile : public TransparentTile
{
public:
	PortalTile(int_t id, int_t texture);
	
	// Beta: PortalTile.getAABB() - returns null (no collision) (PortalTile.java:16-18)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PortalTile.updateShape() - updates shape based on orientation (PortalTile.java:21-31)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PortalTile.isCubeShaped() - returns false (PortalTile.java:39-41)
	virtual bool isCubeShaped() override;
	
	// Beta: PortalTile.neighborChanged() - validates portal structure (PortalTile.java:93-129)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: PortalTile.shouldRenderFace() - always returns true (PortalTile.java:132-134)
	virtual bool shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: PortalTile.getResourceCount() - returns 0 (PortalTile.java:137-139)
	virtual int_t getResourceCount(Random &random) override;
	
	// Beta: PortalTile.getRenderLayer() - returns 1 (PortalTile.java:142-144)
	virtual int_t getRenderLayer() override;
	
	// Beta: PortalTile.entityInside() - handles portal teleportation (PortalTile.java:147-151)
	virtual void entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
	
	// Beta: PortalTile.animateTick() - spawns portal particles (PortalTile.java:154-180)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: PortalTile.trySpawnPortal() - attempts to create portal structure (PortalTile.java:43-90)
	// Note: This is called from FireTile when fire is placed in obsidian frame
	bool trySpawnPortal(Level &level, int_t x, int_t y, int_t z);
};
