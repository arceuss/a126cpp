#include "world/item/ItemFishingRod.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/entity/projectile/FishingHook.h"
#include "util/Memory.h"

// Beta: FishingRodItem(int var1) (FishingRodItem.java:8-11)
// Alpha: ItemFishingRod(int var1) (ItemFishingRod.java:4-7)
ItemFishingRod::ItemFishingRod(int_t id) : Item(id)
{
	// Beta: this.maxDamage = 64 (FishingRodItem.java:10)
	// Alpha: this.maxDamage = 64 (ItemFishingRod.java:6)
	setMaxDamage(64);
}

// Beta: use() - casts or retrieves fishing hook (FishingRodItem.java:24-39)
// Alpha: onItemRightClick() - same logic (ItemFishingRod.java:17-32)
ItemStack ItemFishingRod::use(ItemStack &stack, Level &level, Player &player)
{
	// Beta: If already fishing, retrieve hook (FishingRodItem.java:25-28)
	if (player.fishing != nullptr)
	{
		int_t damage = player.fishing->retrieve();
		stack.damageItem(damage);
		player.swing();
	}
	else
	{
		// Beta: Cast fishing hook (FishingRodItem.java:30-36)
		level.playSound(&player, u"random.bow", 0.5f, 0.4f / (level.random.nextFloat() * 0.4f + 0.8f));
		if (!level.isOnline)
		{
			auto hook = Util::make_shared<FishingHook>(level, player);
			level.addEntity(hook);
		}
		player.swing();
	}
	
	return stack;
}
