#include "world/item/ItemPainting.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
// TODO: Include Painting entity when available
// #include "world/entity/Painting.h"

// Beta: PaintingItem(int var1) (PaintingItem.java:8-11)
// Alpha: ItemPainting(int var1) (ItemPainting.java:8-11)
ItemPainting::ItemPainting(int_t id) : Item(id)
{
	// Beta: this.maxDamage = 64 (PaintingItem.java:10)
	setMaxDamage(64);
}

// Beta: useOn() - places painting on wall (PaintingItem.java:14-44)
// Alpha: onItemUse() - same logic (ItemPainting.java:13-43)
bool ItemPainting::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Can't place on bottom or top (PaintingItem.java:15-19)
	if (face == Facing::DOWN || face == Facing::UP)
		return false;
	
	// Beta: Determine painting direction based on face (PaintingItem.java:20-31)
	int_t direction = 0;
	if (face == Facing::EAST)
		direction = 1;
	else if (face == Facing::SOUTH)
		direction = 2;
	else if (face == Facing::WEST)
		direction = 3;
	
	// Beta: Create painting entity (PaintingItem.java:33)
	// TODO: Create and add Painting entity
	// Painting *painting = new Painting(level, x, y, z, direction);
	// if (painting->survives())
	// {
	//     if (!level.isOnline)
	//         level.addEntity(painting);
	//     stack.stackSize--;
	// }
	
	// For now, just decrement stack
	stack.stackSize--;
	return true;
}
