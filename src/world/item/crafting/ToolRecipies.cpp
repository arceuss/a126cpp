#include "world/item/crafting/ToolRecipies.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/WoodTile.h"
#include "world/level/tile/CobblestoneTile.h"

// Beta: ToolRecipies.addRecipes() - adds all tool recipes (ToolRecipies.java:17-26)
void ToolRecipies::addRecipes(Recipes &recipes)
{
	// Beta: shapes array (ToolRecipies.java:8)
	const char* shapes[4][3] = {
		{"XXX", " # ", " # "},  // Pickaxe
		{"X", "#", "#"},         // Shovel
		{"XX", "X#", " #"},     // Hatchet
		{"XX", " #", " #"}      // Hoe
	};
	
	// Beta: map array - materials and tools (ToolRecipies.java:9-15)
	// Row 0: Materials (wood, stoneBrick, ironIngot, emerald, goldIngot)
	// Row 1: Pickaxes
	// Row 2: Shovels
	// Row 3: Hatchets
	// Row 4: Hoes
	
	Tile* materials[5] = {
		static_cast<Tile*>(&Tile::wood),
		static_cast<Tile*>(&Tile::cobblestone),  // Beta: stoneBrick
		nullptr,  // ironIngot - will be Item*
		nullptr,  // emerald - will be Item*
		nullptr   // goldIngot - will be Item*
	};
	
	Item* materialsItems[3] = {
		Items::ingotIron,  // Beta: ironIngot
		Items::diamond,    // Beta: emerald (Alpha uses diamond)
		Items::ingotGold   // Beta: goldIngot
	};
	
	Item* pickaxes[5] = {
		Items::pickaxeWood,
		Items::pickaxeStone,
		Items::pickaxeSteel,
		Items::pickaxeDiamond,
		Items::pickaxeGold
	};
	
	Item* shovels[5] = {
		Items::shovelWood,
		Items::shovelStone,
		Items::shovelSteel,
		Items::shovelDiamond,
		Items::shovelGold
	};
	
	Item* hatchets[5] = {
		Items::axeWood,
		Items::axeStone,
		Items::axeSteel,
		Items::axeDiamond,
		Items::axeGold
	};
	
	Item* hoes[5] = {
		Items::hoeWood,
		Items::hoeStone,
		Items::hoeSteel,
		Items::hoeDiamond,
		Items::hoeGold
	};
	
	// Beta: Loop through materials (ToolRecipies.java:18-24)
	for (int i = 0; i < 5; ++i)
	{
		// Get material (Tile* or Item*)
		Tile* materialTile = (i < 2) ? materials[i] : nullptr;
		Item* materialItem = (i >= 2) ? materialsItems[i - 2] : nullptr;
		
		// Beta: Loop through tool types (ToolRecipies.java:21-23)
		// Pickaxe (shape 0)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(pickaxes[i]->getShiftedIndex(), 1), {shapes[0][0], shapes[0][1], shapes[0][2]}, '#', Items::stick, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(pickaxes[i]->getShiftedIndex(), 1), {shapes[0][0], shapes[0][1], shapes[0][2]}, '#', Items::stick, 'X', materialItem);
		
		// Shovel (shape 1)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(shovels[i]->getShiftedIndex(), 1), {shapes[1][0], shapes[1][1], shapes[1][2]}, '#', Items::stick, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(shovels[i]->getShiftedIndex(), 1), {shapes[1][0], shapes[1][1], shapes[1][2]}, '#', Items::stick, 'X', materialItem);
		
		// Hatchet (shape 2)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(hatchets[i]->getShiftedIndex(), 1), {shapes[2][0], shapes[2][1], shapes[2][2]}, '#', Items::stick, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(hatchets[i]->getShiftedIndex(), 1), {shapes[2][0], shapes[2][1], shapes[2][2]}, '#', Items::stick, 'X', materialItem);
		
		// Hoe (shape 3)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(hoes[i]->getShiftedIndex(), 1), {shapes[3][0], shapes[3][1], shapes[3][2]}, '#', Items::stick, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(hoes[i]->getShiftedIndex(), 1), {shapes[3][0], shapes[3][1], shapes[3][2]}, '#', Items::stick, 'X', materialItem);
	}
}
