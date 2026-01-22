#pragma once

#include "world/item/Item.h"

// Beta 1.2: PaintingItem.java
// Alpha 1.2.6: ItemPainting.java
class ItemPainting : public Item
{
public:
	ItemPainting(int_t id);
	
	// Beta: useOn() - places painting on wall (PaintingItem.java:14-44)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
