#include "world/item/ItemPainting.h"

#include <memory>

#include "world/entity/Painting.h"
#include "world/entity/player/Player.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"

ItemPainting::ItemPainting(int_t id) : Item(id)
{
	maxDamage = 64;
}

bool ItemPainting::useOn(ItemStack &stack, Player &player, Level &level,
	int_t x, int_t y, int_t z, Facing face)
{
	(void)player;
	// Direct Alpha transliteration: ItemPainting.java:20-45. Multiplayer
	// returns before face validation and inventory mutation; the server decides.
	if (level.isOnline)
		return true;
	if (face == Facing::DOWN || face == Facing::UP)
		return false;

	int_t direction = 0;
	if (face == Facing::WEST)
		direction = 1;
	if (face == Facing::SOUTH)
		direction = 2;
	if (face == Facing::EAST)
		direction = 3;

	std::shared_ptr<Painting> painting = std::make_shared<Painting>(
		level, x, y, z, direction);
	if (painting->survives())
	{
		level.addEntity(painting);
		--stack.stackSize;
	}
	return true;
}
