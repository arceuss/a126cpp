#pragma once

#include "world/item/Item.h"

// Beta 1.2: FlintAndSteelItem.java
// Alpha 1.2.6: ItemFlintAndSteel.java
class ItemFlintAndSteel : public Item
{
public:
	ItemFlintAndSteel(int_t id);
	
	// Beta: useOn() - lights fire on block (FlintAndSteelItem.java:15-48)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
