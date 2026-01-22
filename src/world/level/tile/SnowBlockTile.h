#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: SnowTile.java - ID 80, texture 66, Material.snow
// This is a full snow block (different from topSnow ID 78)
class SnowBlockTile : public Tile
{
public:
	SnowBlockTile(int_t id, int_t tex);
	
	// Beta: SnowTile.tick() - melts if light > 11 (SnowTile.java:26-31)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: SnowTile.getResource() returns Item.snowBall.id (SnowTile.java:16-18)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: SnowTile.getResourceCount() returns 4 (SnowTile.java:20-23)
	virtual int_t getResourceCount(Random &random) override;
};
