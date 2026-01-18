#pragma once

#include "world/item/Item.h"

// Beta 1.2: SeedItem.java
// Alpha 1.2.6: ItemSeeds.java
class ItemSeeds : public Item
{
private:
	int_t resultId;  // Block ID to place (crops)

public:
	ItemSeeds(int_t id, int_t resultId);
	
	// Beta: useOn() - plants seeds on farmland (SeedItem.java:16-29)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
