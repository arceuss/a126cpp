#include "world/item/crafting/WeaponRecipies.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/WoodTile.h"
#include "world/level/tile/CobblestoneTile.h"

// Beta: WeaponRecipies.addRecipes() - adds all weapon recipes (WeaponRecipies.java:14-26)
void WeaponRecipies::addRecipes(Recipes &recipes)
{
	// Beta: shapes array (WeaponRecipies.java:8)
	const char* shapes[1][3] = {
		{"X", "X", "#"}  // Sword
	};
	
	// Beta: map array - materials and swords (WeaponRecipies.java:9-12)
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
	
	Item* swords[5] = {
		Items::swordWood,
		Items::swordStone,
		Items::swordSteel,
		Items::swordDiamond,
		Items::swordGold
	};
	
	// Beta: Loop through materials (WeaponRecipies.java:15-21)
	for (int i = 0; i < 5; ++i)
	{
		Tile* materialTile = (i < 2) ? materials[i] : nullptr;
		Item* materialItem = (i >= 2) ? materialsItems[i - 2] : nullptr;
		
		// Beta: Add sword recipe (WeaponRecipies.java:20)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(swords[i]->getShiftedIndex(), 1), {shapes[0][0], shapes[0][1], shapes[0][2]}, '#', Items::stick, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(swords[i]->getShiftedIndex(), 1), {shapes[0][0], shapes[0][1], shapes[0][2]}, '#', Items::stick, 'X', materialItem);
	}
	
	// Beta: Bow recipe (WeaponRecipies.java:24)
	recipes.addShapedRecipy(ItemStack(Items::bow->getShiftedIndex(), 1), {" #X", "# X", " #X"}, 'X', Items::silk, '#', Items::stick);
	
	// Beta: Arrow recipe (WeaponRecipies.java:25)
	recipes.addShapedRecipy(ItemStack(Items::arrow->getShiftedIndex(), 4), {"X", "#", "Y"}, 'Y', Items::feather, 'X', Items::flint, '#', Items::stick);
}
