#pragma once

#include "world/item/Item.h"

// Beta 1.2: RedStoneItem.java
// Alpha 1.2.6: ItemRedstone.java
class ItemRedstone : public Item
{
public:
	ItemRedstone(int_t id);
	
	// Beta: useOn() - places redstone dust (RedStoneItem.java:13-49)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
