#include "world/item/crafting/OreRecipies.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/MetalTile.h"

// Beta: OreRecipies.addRecipes() - adds all ore recipes (OreRecipies.java:15-22)
void OreRecipies::addRecipes(Recipes &recipes)
{
	// Beta: map array - blocks and ingots (OreRecipies.java:8-13)
	Tile* blocks[4] = {
		static_cast<Tile*>(&Tile::goldBlock),
		static_cast<Tile*>(&Tile::ironBlock),
		static_cast<Tile*>(&Tile::emeraldBlock),
		nullptr  // lapisBlock - check if exists in Alpha
	};
	
	ItemStack ingots[4] = {
		ItemStack(Items::ingotGold->getShiftedIndex(), 9),
		ItemStack(Items::ingotIron->getShiftedIndex(), 9),
		ItemStack(Items::diamond->getShiftedIndex(), 9),  // Beta: emerald
		ItemStack(0, 0)  // lapis - not in Alpha 1.2.6
	};
	
	// Beta: Loop through blocks (OreRecipies.java:16-21)
	for (int i = 0; i < 3; ++i)  // Only first 3 exist in Alpha
	{
		Tile* block = blocks[i];
		ItemStack ingot = ingots[i];
		
		// Beta: Block from ingots (OreRecipies.java:19)
		recipes.addShapedRecipy(ItemStack(block->id, 1), {"###", "###", "###"}, '#', ingot);
		
		// Beta: Ingot from block (OreRecipies.java:20)
		recipes.addShapedRecipy(ingot, {"#"}, '#', static_cast<Tile*>(block));
	}
}
