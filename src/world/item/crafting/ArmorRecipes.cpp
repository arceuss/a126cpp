#include "world/item/crafting/ArmorRecipes.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FireTile.h"

// Beta: ArmorRecipes.addRecipes() - adds all armor recipes (ArmorRecipes.java:17-26)
void ArmorRecipes::addRecipes(Recipes &recipes)
{
	// Beta: shapes array (ArmorRecipes.java:8)
	const char* shapes[4][3] = {
		{"XXX", "X X"},           // Helmet
		{"X X", "XXX", "XXX"},    // Chestplate
		{"XXX", "X X", "X X"},    // Leggings
		{"X X", "X X"}            // Boots
	};
	
	// Beta: map array - materials and armor pieces (ArmorRecipes.java:9-15)
	// Row 0: Materials (leather, fire, ironIngot, emerald, goldIngot)
	// Row 1: Helmets
	// Row 2: Chestplates
	// Row 3: Leggings
	// Row 4: Boots
	
	Item* materials[5] = {
		Items::leather,   // Beta: leather
		nullptr,          // Beta: fire (Tile::fire) - will be Tile*
		Items::ingotIron, // Beta: ironIngot
		Items::diamond,   // Beta: emerald (Alpha uses diamond)
		Items::ingotGold  // Beta: goldIngot
	};
	
	Item* helmets[5] = {
		Items::helmetLeather,
		Items::helmetChain,
		Items::helmetSteel,
		Items::helmetDiamond,
		Items::helmetGold
	};
	
	Item* chestplates[5] = {
		Items::plateLeather,
		Items::plateChain,
		Items::plateSteel,
		Items::plateDiamond,
		Items::plateGold
	};
	
	Item* leggings[5] = {
		Items::legsLeather,
		Items::legsChain,
		Items::legsSteel,
		Items::legsDiamond,
		Items::legsGold
	};
	
	Item* boots[5] = {
		Items::bootsLeather,
		Items::bootsChain,
		Items::bootsSteel,
		Items::bootsDiamond,
		Items::bootsGold
	};
	
	// Beta: Loop through materials (ArmorRecipes.java:18-24)
	for (int i = 0; i < 5; ++i)
	{
		Item* materialItem = (i == 0 || i >= 2) ? materials[i] : nullptr;
		Tile* materialTile = (i == 1) ? static_cast<Tile*>(&Tile::fire) : nullptr;
		
		// Beta: Loop through armor types (ArmorRecipes.java:21-23)
		// Helmet (shape 0)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(helmets[i]->getShiftedIndex(), 1), {shapes[0][0], shapes[0][1]}, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(helmets[i]->getShiftedIndex(), 1), {shapes[0][0], shapes[0][1]}, 'X', materialItem);
		
		// Chestplate (shape 1)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(chestplates[i]->getShiftedIndex(), 1), {shapes[1][0], shapes[1][1], shapes[1][2]}, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(chestplates[i]->getShiftedIndex(), 1), {shapes[1][0], shapes[1][1], shapes[1][2]}, 'X', materialItem);
		
		// Leggings (shape 2)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(leggings[i]->getShiftedIndex(), 1), {shapes[2][0], shapes[2][1], shapes[2][2]}, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(leggings[i]->getShiftedIndex(), 1), {shapes[2][0], shapes[2][1], shapes[2][2]}, 'X', materialItem);
		
		// Boots (shape 3)
		if (materialTile)
			recipes.addShapedRecipy(ItemStack(boots[i]->getShiftedIndex(), 1), {shapes[3][0], shapes[3][1]}, 'X', materialTile);
		else
			recipes.addShapedRecipy(ItemStack(boots[i]->getShiftedIndex(), 1), {shapes[3][0], shapes[3][1]}, 'X', materialItem);
	}
}
