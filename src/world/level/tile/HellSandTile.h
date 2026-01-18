#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: HellSandTile.java - ID 88, texture 89
// Alpha: Block.slowSand (Block.java:109)
// HellSandTile uses Material.sand, has custom AABB and entityInside
class HellSandTile : public Tile
{
public:
	HellSandTile(int_t id, int_t texture);
	
	// Beta: HellSandTile.getAABB() - custom collision box (HellSandTile.java:14-16)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: HellSandTile.entityInside() - slows entities (HellSandTile.java:20-22)
	virtual void entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
};
