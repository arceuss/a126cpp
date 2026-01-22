#pragma once

#include "world/item/Item.h"

// Beta: TileItem class - item for placing blocks (TileItem.java)
class TileItem : public Item
{
private:
	int_t tileId;

public:
	TileItem(int_t baseTileId);
	
	int_t getTileId() const { return tileId; }
	
	// Beta: TileItem.useOn() - places block (TileItem.java:20-72)
	bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
