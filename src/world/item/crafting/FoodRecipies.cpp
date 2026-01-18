#include "world/item/crafting/FoodRecipies.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/MushroomTile.h"

// Beta: FoodRecipies.addRecipes() - adds all food recipes (FoodRecipies.java:8-11)
void FoodRecipies::addRecipes(Recipes &recipes)
{
	// Beta: Mushroom stew recipe - two variants (FoodRecipies.java:9-10)
	recipes.addShapedRecipy(ItemStack(Items::bowlSoup->getShiftedIndex(), 1), {"Y", "X", "#"}, 'X', static_cast<Tile*>(&Tile::mushroomBrown), 'Y', static_cast<Tile*>(&Tile::mushroomRed), '#', Items::bowlEmpty);
	recipes.addShapedRecipy(ItemStack(Items::bowlSoup->getShiftedIndex(), 1), {"Y", "X", "#"}, 'X', static_cast<Tile*>(&Tile::mushroomRed), 'Y', static_cast<Tile*>(&Tile::mushroomBrown), '#', Items::bowlEmpty);
}
