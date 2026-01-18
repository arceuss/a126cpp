#pragma once

#include "world/item/Item.h"

// Beta 1.2: TilePlanterItem.java
// Alpha 1.2.6: ItemReed.java
class ItemReed : public Item
{
private:
	int_t tileId;  // Beta: tileId (TilePlanterItem.java:8), Alpha: blockID (ItemReed.java:4)

public:
	ItemReed(int_t id, int_t tileId);
	
	// Beta: useOn() - places reed (TilePlanterItem.java:16-66)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
