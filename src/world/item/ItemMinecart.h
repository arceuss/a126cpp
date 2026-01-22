#pragma once

#include "world/item/Item.h"

// Beta 1.2: MinecartItem.java
// Alpha 1.2.6: ItemMinecart.java
class ItemMinecart : public Item
{
public:
	int_t type;  // Beta: type (MinecartItem.java:9), Alpha: field_317_a (ItemMinecart.java:4)
	             // 0 = empty, 1 = chest, 2 = powered

	ItemMinecart(int_t id, int_t type);
	
	// Beta: useOn() - places minecart on rail (MinecartItem.java:18-30)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
