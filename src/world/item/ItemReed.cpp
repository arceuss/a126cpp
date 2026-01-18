#include "world/item/ItemReed.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SnowTile.h"
#include "world/level/tile/ReedTile.h"
#include "Facing.h"

// Beta: TilePlanterItem(int var1, Tile var2) (TilePlanterItem.java:10-13)
// Alpha: ItemReed(int var1, int var2) (ItemReed.java:6-10)
ItemReed::ItemReed(int_t id, int_t tileId) : Item(id), tileId(tileId)
{
	// Beta: this.tileId = var2.id (TilePlanterItem.java:12)
}

// Beta: useOn() - places reed (TilePlanterItem.java:16-66)
// Alpha: onItemUse() - same logic (ItemReed.java:12-26)
bool ItemReed::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Special case for top snow (TilePlanterItem.java:17-18)
	if (level.getTile(x, y, z) == Tile::snow.id)
		face = Facing::DOWN;
	else
	{
		// Beta: Adjust coordinates based on face (TilePlanterItem.java:20-42)
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
	}
	
	// Beta: Check if stack is empty (TilePlanterItem.java:45-47)
	if (stack.stackSize == 0)
		return false;
	
	// Beta: Check if can place (TilePlanterItem.java:48)
	if (level.mayPlace(tileId, x, y, z, false))
	{
		Tile *tile = Tile::tiles[tileId];
		// Beta: Place tile (TilePlanterItem.java:50-61)
		if (level.setTile(x, y, z, tileId))
		{
			tile->setPlacedOnFace(level, x, y, z, face);
			level.playSound(
				(double)x + 0.5,
				(double)y + 0.5,
				(double)z + 0.5,
				tile->getSoundType()->getStepSound(),
				(tile->getSoundType()->getVolume() + 1.0f) / 2.0f,
				tile->getSoundType()->getPitch() * 0.8f
			);
			stack.stackSize--;
		}
	}
	
	return true;
}
