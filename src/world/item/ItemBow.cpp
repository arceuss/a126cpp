#include "world/item/ItemBow.h"

#include <memory>

#include "world/entity/player/Player.h"
#include "world/entity/projectile/Arrow.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "world/level/Level.h"

ItemBow::ItemBow(int_t id) : Item(id)
{
	maxStackSize = 1;
}

ItemStack ItemBow::use(ItemStack &stack, Level &level, Player &player)
{
	// Direct Alpha transliteration: ItemBow.java:20-25. consumeInventoryItem is
	// evaluated before multiplayerWorld because Java && evaluates left-to-right.
	if (player.inventory.consumeInventoryItem(Items::arrow->getShiftedIndex()) && !level.isOnline)
	{
		level.playSound(&player, u"random.bow", 1.0f,
			1.0f / (itemRand.nextFloat() * 0.4f + 0.8f));
		level.addEntity(std::make_shared<Arrow>(level, player));
	}
	return stack;
}
