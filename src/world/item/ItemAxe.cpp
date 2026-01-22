#include "world/item/ItemAxe.h"
#include "world/level/tile/Tile.h"
#include <vector>

// Alpha: Blocks effective against axe (ItemAxe.java:4)
// Full list: planks (5), bookShelf (47), wood (17), crate (54)
// Note: Only including blocks that exist in Alpha 1.2.6
static const std::vector<int_t> AXE_EFFECTIVE_BLOCKS = {
	5,   // wood/planks
	17,  // log (treeTrunk)
	47,  // bookshelf
	51,  // woodStairs
	52,  // chest
	54,  // workbench
	56,  // sign (wood)
	62,  // doorWood
	69,  // pressurePlate_wood
	80,  // jukebox
	81   // fence
};

ItemAxe::ItemAxe(int_t id, int_t tier)
	: ItemTool(id, 3, tier, AXE_EFFECTIVE_BLOCKS)
{
}
