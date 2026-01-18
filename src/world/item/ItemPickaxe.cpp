#include "world/item/ItemPickaxe.h"
#include "world/level/tile/Tile.h"
#include "world/level/material/Material.h"
#include <vector>

// Alpha: Blocks effective against pickaxe (ItemPickaxe.java:4)
// Full list: cobblestone (4), stairDouble (43), stairSingle (44), stone (1), cobblestoneMossy (48),
//            oreIron (15), blockSteel (42), oreCoal (16), blockGold (41), oreGold (14),
//            oreDiamond (56), blockDiamond (57), blockIce (79), bloodStone (87)
// Note: Only including blocks that exist in Alpha 1.2.6
static const std::vector<int_t> PICKAXE_EFFECTIVE_BLOCKS = {
	1,   // stone
	4,   // cobblestone
	14,  // oreGold
	15,  // oreIron
	16,  // oreCoal
	41,  // blockGold
	42,  // blockSteel (iron block)
	43,  // stairDouble (stone slab double)
	44,  // stairSingle (stone slab)
	48,  // cobblestoneMossy
	49,  // obsidian
	56,  // oreDiamond
	57,  // blockDiamond
	// Note: Furnace (61, 62) not in Beta's diggables array - relies on Material.stone check
	65,  // stoneButton
	67,  // pressurePlate_stone
	73,  // redStoneOre
	74,  // redStoneOre_lit
	75,  // redstoneTorch_off (notGate_off)
	76,  // redstoneTorch_on (notGate_on)
	79,  // ice
	87   // hellRock (bloodStone)
};

ItemPickaxe::ItemPickaxe(int_t id, int_t tier)
	: ItemTool(id, 2, tier, PICKAXE_EFFECTIVE_BLOCKS)
{
	this->harvestLevel = tier;
}

bool ItemPickaxe::canHarvestBlock(Tile &tile)
{
	// Alpha: canHarvestBlock logic (ItemPickaxe.java:12-13)
	// Obsidian requires tier 3 (diamond)
	if (tile.id == 49) {  // obsidian
		return harvestLevel == 3;
	}
	
	// Diamond/gold blocks/ores require tier >= 2 (iron/diamond)
	if (tile.id == 41 || tile.id == 14 || tile.id == 57 || tile.id == 56) {  // blockGold, oreGold, blockDiamond, oreDiamond
		return harvestLevel >= 2;
	}
	
	// Iron/steel blocks/ores require tier >= 1 (stone+)
	if (tile.id == 42 || tile.id == 15) {  // blockSteel, oreIron
		return harvestLevel >= 1;
	}
	
	// Redstone ores require tier >= 2
	if (tile.id == 73 || tile.id == 74) {  // redStoneOre, redStoneOre_lit
		return harvestLevel >= 2;
	}
	
	// Rock and iron materials can be harvested by any tier
	if (tile.material == Material::stone || tile.material == Material::metal) {
		return true;
	}
	
	return false;
}
