#include "world/item/ItemSoup.h"

#include "world/item/ItemStack.h"
#include "world/item/Items.h"

ItemSoup::ItemSoup(int_t id, int_t healAmount) : ItemFood(id, healAmount)
{
}

ItemStack ItemSoup::use(ItemStack &stack, Level &level, Player &player)
{
	// Direct Alpha transliteration: ItemSoup.java:18-20.
	ItemFood::use(stack, level, player);
	return ItemStack(Items::bowlEmpty->getShiftedIndex(), 1);
}
