#pragma once

#include "world/item/Item.h"

// Beta 1.2: BowItem.java
// Alpha 1.2.6: ItemBow.java
class ItemBow : public Item
{
public:
	ItemBow(int_t id);
	
	// Beta: use() - fires arrow (BowItem.java:14-23)
	virtual ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
