#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: CactusTile.java - ID 81, texture 70
// CactusTile uses Material.cactus
class CactusTile : public Tile
{
public:
	CactusTile(int_t id, int_t tex);
	
	// Beta: CactusTile.tick() - grows upward (CactusTile.java:16-34)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: CactusTile.mayPlace() - requires sand/cactus below and no adjacent blocks (CactusTile.java:73-75)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: CactusTile.neighborChanged() - checks canSurvive, removes if not (CactusTile.java:78-83)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: CactusTile.entityInside() - hurts entities (CactusTile.java:102-104)
	virtual void entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
	
	// Beta: CactusTile.getRenderShape() returns 13 (CactusTile.java:68-70)
	virtual Shape getRenderShape() override;
	
	// Beta: CactusTile.isCubeShaped() returns false (CactusTile.java:58-60)
	virtual bool isCubeShaped() override;
	
	// Beta: CactusTile.isSolidRender() returns false (CactusTile.java:63-64)
	virtual bool isSolidRender() override;
	
	// Beta: CactusTile.getTexture() returns different textures for top/bottom (CactusTile.java:49-54)
	virtual int_t getTexture(Facing face) override;
	
	// Beta: CactusTile.getAABB() - custom collision box (CactusTile.java:37-40)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: CactusTile.getTileAABB() - custom collision box for tile (CactusTile.java:43-46)
	virtual AABB *getTileAABB(Level &level, int_t x, int_t y, int_t z) override;
	
protected:
	// Beta: CactusTile.canSurvive() - checks sand/cactus below and no solid adjacent blocks (CactusTile.java:86-99)
	bool canSurvive(Level &level, int_t x, int_t y, int_t z);
};
