#pragma once

#include "world/item/Item.h"

// Beta 1.2: HoeItem.java
// Alpha 1.2.6: ItemHoe.java
class ItemHoe : public Item
{
private:
	int_t tier;  // Tool tier (0=wood/gold, 1=stone, 2=iron/steel, 3=diamond)

public:
	ItemHoe(int_t id, int_t tier);
	
	// Beta: useOn() - tills dirt/grass into farmland (HoeItem.java:17-54)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: isHandEquipped() - returns true (HoeItem.java:57-59)
	// Alpha: isFull3D() - returns true (ItemHoe.java:42-44)
	virtual bool isHandEquipped() const override { return true; }
	virtual bool isFull3D() const override { return true; }
};
