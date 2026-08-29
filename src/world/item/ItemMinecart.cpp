#include "world/item/ItemMinecart.h"

#include <memory>

#include "Facing.h"
#include "world/entity/item/Minecart.h"
#include "world/entity/player/Player.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"
#include "world/level/tile/RailTile.h"
#include "world/level/tile/Tile.h"

// Beta: MinecartItem(int var1, int var2) (MinecartItem.java:11-15)
// Alpha: ItemMinecart(int var1, int var2) (ItemMinecart.java:6-10)
ItemMinecart::ItemMinecart(int_t id, int_t type) : Item(id), type(type)
{
	// Beta: this.maxStackSize = 1 (MinecartItem.java:13)
	setMaxStackSize(1);
}

// Beta: useOn() - places minecart on rail (MinecartItem.java:18-30)
// Alpha: onItemUse() - same logic (ItemMinecart.java:12-24)
bool ItemMinecart::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Check if block is rail (MinecartItem.java:19-29)
	int_t tileId = level.getTile(x, y, z);
	if (tileId == Tile::rail.id)
	{
		// Direct Alpha transliteration: ItemMinecart.java:25-30.
		if (!level.isOnline)
		{
			level.addEntity(std::make_shared<Minecart>(level,
				static_cast<double>(x) + 0.5,
				static_cast<double>(y) + 0.5,
				static_cast<double>(z) + 0.5, type));
		}
		
		// Beta: Decrement stack (MinecartItem.java:25)
		stack.stackSize--;
		return true;
	}
	
	return false;
}
