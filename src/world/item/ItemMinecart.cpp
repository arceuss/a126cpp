#include "world/item/ItemMinecart.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/RailTile.h"
// TODO: Include Minecart entity when available
// #include "world/entity/item/Minecart.h"
#include "Facing.h"

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
		// Beta: Spawn minecart if not online (MinecartItem.java:21-23)
		if (!level.isOnline)
		{
			// TODO: Create and add Minecart entity
			// Minecart *minecart = new Minecart(level, (double)x + 0.5, (double)y + 0.5, (double)z + 0.5, type);
			// level.addEntity(minecart);
		}
		
		// Beta: Decrement stack (MinecartItem.java:25)
		stack.stackSize--;
		return true;
	}
	
	return false;
}
