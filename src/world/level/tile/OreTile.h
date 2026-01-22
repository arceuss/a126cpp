#pragma once

#include "world/level/tile/Tile.h"

// Alpha 1.2.6 BlockOre (coal, iron, gold, diamond)
// Reference: apclient/minecraft/src/net/minecraft/src/BlockOre.java
class OreTile : public Tile
{
private:
	int_t dropItemId;  // Item ID to drop (for coal/diamond), or 0 to drop self

public:
	OreTile(int_t id, int_t tex, int_t dropId = 0);
	
	// Alpha: BlockOre.idDropped() - coal drops Item.coal, diamond drops Item.diamond, others drop self (BlockOre.java:10-11)
	virtual int_t getResource(int_t data, Random &random) override;
};
