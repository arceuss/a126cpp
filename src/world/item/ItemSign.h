#pragma once

#include "world/item/Item.h"

// Beta 1.2: SignItem.java
// Alpha 1.2.6: ItemSign.java
class ItemSign : public Item
{
public:
	ItemSign(int_t id);
	
	// Beta: useOn() - places sign (SignItem.java:17-61)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
