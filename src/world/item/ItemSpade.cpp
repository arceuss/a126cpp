#include "world/item/ItemSpade.h"
#include "world/level/tile/Tile.h"
#include <vector>

// Alpha: Blocks effective against shovel (ItemSpade.java:4)
// Full list: grass (2), dirt (3), sand (12), gravel (13), snow (78), blockSnow (80), blockClay (82)
// Note: Only including blocks that exist in Alpha 1.2.6
static const std::vector<int_t> SHOVEL_EFFECTIVE_BLOCKS = {
	2,   // grass
	3,   // dirt
	12,  // sand
	13,  // gravel
	58,  // farmland
	78,  // snow
	80,  // blockSnow
	82,  // clay
	88   // hellSand
};

ItemSpade::ItemSpade(int_t id, int_t tier)
	: ItemTool(id, 1, tier, SHOVEL_EFFECTIVE_BLOCKS)
{
}

bool ItemSpade::canHarvestBlock(Tile &tile)
{
	// Alpha: canHarvestBlock only returns true for snow blocks (ItemSpade.java:10-11)
	// All other blocks can be broken but don't require shovel
	if (tile.id == 78 || tile.id == 80) {  // snow, blockSnow
		return true;
	}
	return false;
}
