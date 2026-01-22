#include "world/item/ItemSeeds.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/tile/CropTile.h"
#include "Facing.h"

// Beta: SeedItem(int var1, int var2) (SeedItem.java:10-13)
// Alpha: ItemSeeds(int var1, int var2) (ItemSeeds.java:10-13)
ItemSeeds::ItemSeeds(int_t id, int_t resultId) : Item(id), resultId(resultId)
{
	// Beta: this.resultId = var2 (SeedItem.java:12)
}

// Beta: useOn() - plants seeds on farmland (SeedItem.java:16-29)
// Alpha: onItemUse() - same logic (ItemSeeds.java:15-28)
bool ItemSeeds::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Can only plant on top face (SeedItem.java:17-19)
	if (face != Facing::UP)
		return false;
	
	// Beta: Check if farmland and air above (SeedItem.java:20-27)
	int_t tileId = level.getTile(x, y, z);
	if (tileId == Tile::farmland.id && level.isEmptyTile(x, y + 1, z))
	{
		// Beta: Place crop block (SeedItem.java:22)
		level.setTile(x, y + 1, z, resultId);
		
		// Beta: Decrement stack (SeedItem.java:23)
		stack.stackSize--;
		
		return true;
	}
	
	return false;
}
