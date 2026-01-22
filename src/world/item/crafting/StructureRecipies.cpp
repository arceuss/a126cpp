#include "world/item/crafting/StructureRecipies.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/WoodTile.h"
#include "world/level/tile/CobblestoneTile.h"
#include "world/level/tile/ChestTile.h"
#include "world/level/tile/FurnaceTile.h"
#include "world/level/tile/WorkbenchTile.h"

// Beta: StructureRecipies.addRecipes() - adds all structure recipes (StructureRecipies.java:7-12)
void StructureRecipies::addRecipes(Recipes &recipes)
{
	// Beta: Chest recipe (StructureRecipies.java:8)
	recipes.addShapedRecipy(ItemStack(Tile::chest.id, 1), {"###", "# #", "###"}, '#', static_cast<Tile*>(&Tile::wood));
	
	// Beta: Furnace recipe (StructureRecipies.java:9)
	recipes.addShapedRecipy(ItemStack(Tile::furnace.id, 1), {"###", "# #", "###"}, '#', static_cast<Tile*>(&Tile::cobblestone));
	
	// Beta: Workbench recipe (StructureRecipies.java:10)
	recipes.addShapedRecipy(ItemStack(Tile::workBench.id, 1), {"##", "##"}, '#', static_cast<Tile*>(&Tile::wood));
	
	// Beta: Sandstone recipe - SKIP (Tile.sandStone doesn't exist in Alpha 1.2.6)
	// addShapedRecipy(ItemStack(Tile.sandStone.id, 1), {"##", "##"}, '#', &Tile::sand);
}
