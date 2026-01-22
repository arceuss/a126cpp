#pragma once

#include "world/item/Item.h"

// Beta 1.2: BoatItem.java
// Alpha 1.2.6: ItemBoat.java
class ItemBoat : public Item
{
public:
	ItemBoat(int_t id);
	
	// Beta: use() - places boat (BoatItem.java:17-50)
	virtual ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
