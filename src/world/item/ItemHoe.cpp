#include "world/item/ItemHoe.h"

#include <memory>
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/material/Material.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/Items.h"
#include "Facing.h"
#include "java/Random.h"

// Beta: HoeItem(int var1, Item.Tier var2) (HoeItem.java:10-14)
// Alpha: ItemHoe(int var1, int var2) (ItemHoe.java:4-8)
ItemHoe::ItemHoe(int_t id, int_t tier) : Item(id), tier(tier)
{
	// Direct Alpha transliteration: ItemHoe.java:16-20.
	maxStackSize = 1;
	maxDamage = 32 << tier;
}

// Beta: useOn() - tills dirt/grass into farmland (HoeItem.java:17-54)
// Alpha: onItemUse() - same logic (ItemHoe.java:10-40)
bool ItemHoe::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Get tile and material above (HoeItem.java:18-19)
	int_t tileId = level.getTile(x, y, z);
	const Material &materialAbove = level.getMaterial(x, y + 1, z);
	
	// Beta: Check if can till (HoeItem.java:20-21)
	// Can only till grass or dirt, and nothing solid above
	if ((materialAbove.isSolid() || tileId != Tile::grass.id) && tileId != Tile::dirt.id)
		return false;
	
	Tile &farmland = Tile::farmland;
	// Alpha returns before sound, block mutation, durability and RNG on a
	// multiplayer world (ItemHoe.java:29-32).
	if (level.isOnline)
		return true;

	level.playSound(
		static_cast<double>(x) + 0.5,
		static_cast<double>(y) + 0.5,
		static_cast<double>(z) + 0.5,
		farmland.getSoundType()->getStepSound(),
		(farmland.getSoundType()->getVolume() + 1.0f) / 2.0f,
		farmland.getSoundType()->getPitch() * 0.8f);
	level.setTile(x, y, z, farmland.id);
	stack.damageItem(1);

	if (level.random.nextInt(8) == 0 && tileId == Tile::grass.id)
	{
		// ItemHoe.java:37-46 has a literal one-iteration loop.
		const int_t count = 1;
		for (int_t i = 0; i < count; ++i)
		{
			const float spread = 0.7f;
			const float offsetX = level.random.nextFloat() * spread
				+ (1.0f - spread) * 0.5f;
			const float offsetY = 1.2f;
			const float offsetZ = level.random.nextFloat() * spread
				+ (1.0f - spread) * 0.5f;
			ItemStack seeds(Items::seeds->getShiftedIndex(), 1);
			std::shared_ptr<EntityItem> item = std::make_shared<EntityItem>(
				level, static_cast<double>(x) + offsetX,
				static_cast<double>(y) + offsetY,
				static_cast<double>(z) + offsetZ, seeds);
			item->throwTime = 10;
			level.addEntity(item);
		}
	}
	
	return true;
}
