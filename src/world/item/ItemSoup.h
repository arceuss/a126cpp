#pragma once

#include "world/item/ItemFood.h"

class ItemSoup : public ItemFood
{
public:
	ItemSoup(int_t id, int_t healAmount);

	ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
