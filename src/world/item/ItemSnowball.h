#pragma once

#include "world/item/Item.h"

// Beta 1.2: SnowballItem.java
// Alpha 1.2.6: ItemSnowball.java
class ItemSnowball : public Item
{
public:
	ItemSnowball(int_t id);
	
	// Beta: use() - throws snowball (SnowballItem.java:14-22)
	virtual ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
