#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: LightGemTile.java - ID 89, texture 90
// Alpha: Block.lightStone (Block.java:110)
// LightGemTile drops yellowDust item
class LightGemTile : public Tile
{
public:
	LightGemTile(int_t id, int_t texture, const Material &material);
	
	// Beta: LightGemTile.getResource() - returns Item.yellowDust.id (LightGemTile.java:13-15)
	virtual int_t getResource(int_t data, Random &random) override;
};
