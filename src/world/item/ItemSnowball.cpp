#include "world/item/ItemSnowball.h"

#include <memory>

#include "world/entity/player/Player.h"
#include "world/entity/projectile/Snowball.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"

ItemSnowball::ItemSnowball(int_t id) : Item(id)
{
	maxStackSize = 16;
}

ItemStack ItemSnowball::use(ItemStack &stack, Level &level, Player &player)
{
	// Direct Alpha transliteration: ItemSnowball.java:20-26.
	--stack.stackSize;
	if (!level.isOnline)
	{
		level.playSound(&player, u"random.bow", 0.5f,
			0.4f / (itemRand.nextFloat() * 0.4f + 0.8f));
		level.addEntity(std::make_shared<Snowball>(level, player));
	}
	return stack;
}
