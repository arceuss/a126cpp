#include "world/item/ItemBow.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/item/Items.h"
// TODO: Include Arrow entity when available
// #include "world/entity/projectile/Arrow.h"

// Beta: BowItem(int var1) (BowItem.java:8-11)
// Alpha: ItemBow(int var1) (ItemBow.java:8-11)
ItemBow::ItemBow(int_t id) : Item(id)
{
	// Beta: this.maxStackSize = 1 (BowItem.java:10)
	setMaxStackSize(1);
}

// Beta: use() - fires arrow (BowItem.java:14-23)
// Alpha: onItemRightClick() - same logic (ItemBow.java:13-22)
ItemStack ItemBow::use(ItemStack &stack, Level &level, Player &player)
{
	// Beta: Remove arrow from inventory (BowItem.java:15)
	// Alpha: consumeInventoryItem() - removes one arrow from inventory
	// TODO: Implement removeResource in InventoryPlayer
	// if (player.inventory.removeResource(Items::arrow->getShiftedIndex()))
	{
		// Alpha: Play bow sound only in single-player (ItemBow.java:10-11)
		// Java: if(..., !var2.multiplayerWorld) { var2.playSoundAtEntity(...) }
		// In multiplayer, the server sends Packet61DoorChange (effect ID 1002) which plays the sound
		if (!level.isOnline)
		{
			// Alpha: Play bow sound (ItemBow.java:11)
			level.playSound(&player, u"random.bow", 1.0f, 1.0f / (level.random.nextFloat() * 0.4f + 0.8f));
			
			// Beta: Spawn arrow entity if not online (BowItem.java:17-19)
			// Alpha: entityJoinedWorld() adds arrow (ItemBow.java:12)
			// TODO: Create and add Arrow entity
			// Arrow *arrow = new Arrow(level, player);
			// level.addEntity(arrow);
		}
	}
	
	// Beta: Return stack unchanged (BowItem.java:22)
	return stack;
}
