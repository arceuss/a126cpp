#include "world/item/ItemSnowball.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
// TODO: Include Snowball entity when available
// #include "world/entity/projectile/Snowball.h"

// Beta: SnowballItem(int var1) (SnowballItem.java:8-11)
// Alpha: ItemSnowball(int var1) (ItemSnowball.java:8-11)
ItemSnowball::ItemSnowball(int_t id) : Item(id)
{
	// Beta: this.maxStackSize = 16 (SnowballItem.java:10)
	setMaxStackSize(16);
}

// Beta: use() - throws snowball (SnowballItem.java:14-22)
// Alpha: onItemRightClick() - same logic (ItemSnowball.java:13-21)
ItemStack ItemSnowball::use(ItemStack &stack, Level &level, Player &player)
{
	// Beta: Decrement stack (SnowballItem.java:15)
	stack.stackSize--;
	
	// Beta: Play sound (SnowballItem.java:16)
	level.playSound(&player, u"random.bow", 0.5f, 0.4f / (level.random.nextFloat() * 0.4f + 0.8f));
	
	// Beta: Spawn snowball entity if not online (SnowballItem.java:17-19)
	if (!level.isOnline)
	{
		// TODO: Create and add Snowball entity
		// Snowball *snowball = new Snowball(level, player);
		// level.addEntity(snowball);
	}
	
	return stack;
}
