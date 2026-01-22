#include "world/item/ItemRedstone.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/RedStoneDustTile.h"
#include "Facing.h"

// Beta: RedStoneItem(int var1) (RedStoneItem.java:8-10)
// Alpha: ItemRedstone(int var1) (ItemRedstone.java:8-10)
ItemRedstone::ItemRedstone(int_t id) : Item(id)
{
	// Beta: No special properties
}

// Beta: useOn() - places redstone dust (RedStoneItem.java:13-49)
// Alpha: onItemUse() - same logic (ItemRedstone.java:12-48)
bool ItemRedstone::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Adjust coordinates based on face (RedStoneItem.java:14-36)
	if (face == Facing::DOWN)
		y--;
	else if (face == Facing::UP)
		y++;
	else if (face == Facing::NORTH)
		z--;
	else if (face == Facing::SOUTH)
		z++;
	else if (face == Facing::WEST)
		x--;
	else if (face == Facing::EAST)
		x++;
	
	// Beta: Check if target is empty (RedStoneItem.java:38-39)
	if (!level.isEmptyTile(x, y, z))
		return false;
	
	// Beta: Place redstone dust (RedStoneItem.java:41-44)
	if (Tile::redStoneDust.mayPlace(level, x, y, z))
	{
		stack.stackSize--;
		level.setTile(x, y, z, Tile::redStoneDust.id);
	}
	
	return true;
}
