#include "world/item/crafting/Recipes.h"
#include "world/item/crafting/ShapedRecipy.h"
#include "world/item/crafting/ShapelessRecipy.h"
#include "world/item/crafting/ToolRecipies.h"
#include "world/item/crafting/WeaponRecipies.h"
#include "world/item/crafting/OreRecipies.h"
#include "world/item/crafting/FoodRecipies.h"
#include "world/item/crafting/StructureRecipies.h"
#include "world/item/crafting/ArmorRecipes.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/WoodTile.h"
#include "world/level/tile/CobblestoneTile.h"
#include "world/level/tile/FenceTile.h"
#include "world/level/tile/RecordPlayerTile.h"
#include "world/level/tile/BookshelfTile.h"
#include "world/level/tile/SnowTile.h"
#include "world/level/tile/ClayTile.h"
#include "world/level/tile/RedBrickTile.h"
#include "world/level/tile/LightGemTile.h"
#include "world/level/tile/ClothTile.h"
#include "world/level/tile/TntTile.h"
#include "world/level/tile/StoneSlabTile.h"
#include "world/level/tile/LadderTile.h"
#include "world/level/tile/TorchTile.h"
#include "world/level/tile/RailTile.h"
#include "world/level/tile/PumpkinTile.h"
#include "world/level/tile/ChestTile.h"
#include "world/level/tile/FurnaceTile.h"
#include "world/level/tile/StairTile.h"
#include "world/level/tile/LeverTile.h"
#include "world/level/tile/NotGateTile.h"
#include "world/level/tile/ButtonTile.h"
#include "world/level/tile/PressurePlateTile.h"
#include "world/level/tile/WorkbenchTile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/tile/MetalTile.h"
#include "world/level/tile/StoneTile.h"
#include <algorithm>
#include <iostream>

// Helper macro to cast Tile static members to Tile*
#define TILE_PTR(t) static_cast<Tile*>(&Tile::t)

// Beta: getInstance() - singleton access (Recipes.java:18-20)
// Use lazy initialization to ensure Items::initItems() is called first
Recipes &Recipes::getInstance()
{
	static Recipes instance;  // Lazy initialization - created on first access
	return instance;
}

// Beta: getItemFor() - finds matching recipe (Recipes.java:164-173)
ItemStack Recipes::getItemFor(CraftingContainer &container)
{
	for (size_t i = 0; i < recipies.size(); ++i)
	{
		Recipy *recipe = recipies[i].get();
		if (recipe->matches(container))
		{
			return recipe->assemble(container);
		}
	}
	
	return ItemStack();  // Return empty stack (null in Java)
}

// Beta: addShapedRecipy() - adds shaped recipe (Recipes.java:92-142)
void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<std::string> &pattern,
                               const std::vector<std::pair<char, ItemStack>> &mappings)
{
	// Build pattern string
	std::string patternStr;
	int_t width = 0;
	int_t height = (int_t)pattern.size();
	
	for (const auto &row : pattern)
	{
		patternStr += row;
		if (width == 0)
			width = (int_t)row.length();
	}
	
	// Build mapping map
	std::unordered_map<char, ItemStack> charMap;
	for (const auto &pair : mappings)
	{
		charMap[pair.first] = pair.second;
	}
	
	// Build recipe items array
	std::vector<ItemStack> recipeItems(width * height);
	for (int_t i = 0; i < width * height; ++i)
	{
		char c = patternStr[i];
		auto it = charMap.find(c);
		if (it != charMap.end())
		{
			recipeItems[i] = it->second.copy();
		}
		else
		{
			recipeItems[i] = ItemStack();  // Empty stack for null
		}
	}
	
	recipies.push_back(std::make_unique<ShapedRecipy>(width, height, recipeItems, result));
}

// Helper to add shaped recipe from const char* pattern (easier for C++)
void Recipes::addShapedRecipyFromStrings(const ItemStack &result, const std::vector<const char*> &pattern,
                                         const std::vector<std::pair<char, ItemStack>> &mappings)
{
	std::vector<std::string> patternVec;
	for (const char *str : pattern)
	{
		patternVec.push_back(std::string(str));
	}
	addShapedRecipy(result, patternVec, mappings);
}

// Beta: addShapelessRecipy() - adds shapeless recipe (Recipes.java:144-162)
void Recipes::addShapelessRecipy(const ItemStack &result, const std::vector<ItemStack> &ingredients)
{
	std::vector<ItemStack> ingredientCopies;
	for (const auto &ing : ingredients)
	{
		ingredientCopies.push_back(ing.copy());
	}
	recipies.push_back(std::make_unique<ShapelessRecipy>(result, ingredientCopies));
}

// Beta: Recipes constructor - initializes all recipes (Recipes.java:22-90)
Recipes::Recipes()
{
	initRecipes();
	
	// Beta: Sort recipes (Recipes.java:76-88)
	std::sort(recipies.begin(), recipies.end(), [](const std::unique_ptr<Recipy> &a, const std::unique_ptr<Recipy> &b) {
		// Beta: Check if a is Shapeless and b is Shaped (Recipes.java:78-79)
		bool aIsShapeless = dynamic_cast<const ShapelessRecipy*>(a.get()) != nullptr;
		bool bIsShapeless = dynamic_cast<const ShapelessRecipy*>(b.get()) != nullptr;
		bool aIsShaped = dynamic_cast<const ShapedRecipy*>(a.get()) != nullptr;
		bool bIsShaped = dynamic_cast<const ShapedRecipy*>(b.get()) != nullptr;
		
		// Beta: Shapeless after Shaped (Recipes.java:78-79)
		if (aIsShapeless && bIsShaped)
			return false;  // a comes after b (return false means a is not less than b)
		if (aIsShaped && bIsShapeless)
			return true;   // a comes before b (return true means a is less than b)
		
		// Beta: Larger recipes first (Recipes.java:82-85)
		// Note: Java Comparator returns -1 for "a < b", but C++ sort expects true for "a < b"
		// Java: var2.size() < var1.size() returns -1 means "a > b" (a should come before b)
		// C++: return true means "a < b" (a comes before b)
		if (b->size() < a->size())
			return true;   // a comes before b
		if (a->size() < b->size())
			return false;  // a comes after b
		
		return false;  // Equal (Recipes.java:85 returns 0)
	});
	
	// Beta: Print recipe count (Recipes.java:89)
	std::cout << recipies.size() << " recipes" << std::endl;
}

// Helper methods for varargs-style recipe addition (converting Item*/Tile* to ItemStack)
void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key, Item *item)
{
	if (item == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key, ItemStack(item->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key, Tile *tile)
{
	if (tile == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key, ItemStack(tile->id, 1, -1)});  // -1 = any aux value
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key, const ItemStack &stack)
{
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key, stack.copy()});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2)
{
	if (item1 == nullptr || item2 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Item *item2)
{
	if (tile1 == nullptr || item2 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(tile1->id, 1, -1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Tile *tile2)
{
	if (item1 == nullptr || tile2 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(tile2->id, 1, -1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Tile *tile2)
{
	if (tile1 == nullptr || tile2 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(tile1->id, 1, -1)});
	mappings.push_back({key2, ItemStack(tile2->id, 1, -1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, const ItemStack &stack1, char key2, Item *item2)
{
	if (item2 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, stack1.copy()});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, const ItemStack &stack1, char key2, Tile *tile2)
{
	if (tile2 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, stack1.copy()});
	mappings.push_back({key2, ItemStack(tile2->id, 1, -1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Tile *tile2, char key3, Item *item3)
{
	if (tile1 == nullptr || tile2 == nullptr || item3 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(tile1->id, 1, -1)});
	mappings.push_back({key2, ItemStack(tile2->id, 1, -1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3)
{
	if (item1 == nullptr || item2 == nullptr || item3 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Item *item2, char key3, Item *item3)
{
	if (tile1 == nullptr || item2 == nullptr || item3 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(tile1->id, 1, -1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Tile *tile2, char key3, Item *item3)
{
	if (item1 == nullptr || tile2 == nullptr || item3 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(tile2->id, 1, -1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Tile *tile3)
{
	if (item1 == nullptr || item2 == nullptr || tile3 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	mappings.push_back({key3, ItemStack(tile3->id, 1, -1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3, char key4, Item *item4)
{
	if (item1 == nullptr || item2 == nullptr || item3 == nullptr || item4 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	mappings.push_back({key4, ItemStack(item4->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3, char key4, Tile *tile4)
{
	if (item1 == nullptr || item2 == nullptr || item3 == nullptr || tile4 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	mappings.push_back({key4, ItemStack(tile4->id, 1, -1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

void Recipes::addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3, char key4, Item *item4, char key5, Item *item5)
{
	if (item1 == nullptr || item2 == nullptr || item3 == nullptr || item4 == nullptr || item5 == nullptr) return;
	std::vector<std::pair<char, ItemStack>> mappings;
	mappings.push_back({key1, ItemStack(item1->getShiftedIndex(), 1)});
	mappings.push_back({key2, ItemStack(item2->getShiftedIndex(), 1)});
	mappings.push_back({key3, ItemStack(item3->getShiftedIndex(), 1)});
	mappings.push_back({key4, ItemStack(item4->getShiftedIndex(), 1)});
	mappings.push_back({key5, ItemStack(item5->getShiftedIndex(), 1)});
	addShapedRecipyFromStrings(result, pattern, mappings);
}

// Beta: initRecipes() - initializes all recipes in the same order as Java (Recipes.java:22-75)
void Recipes::initRecipes()
{
	// Beta: Recipe helper classes (Recipes.java:23-29)
	ToolRecipies().addRecipes(*this);
	WeaponRecipies().addRecipes(*this);
	OreRecipies().addRecipes(*this);
	FoodRecipies().addRecipes(*this);
	StructureRecipies().addRecipes(*this);
	ArmorRecipes().addRecipes(*this);
	// NOTE: ClothDyeRecipes excluded - not in Alpha 1.2.6
	
	// Beta: Individual recipes (Recipes.java:30-75) - in exact order
	// Beta: Paper recipe (Recipes.java:30)
	addShapedRecipy(ItemStack(Items::paper->getShiftedIndex(), 3), {"###"}, '#', Items::reed);
	
	// Beta: Book recipe (Recipes.java:31)
	addShapedRecipy(ItemStack(Items::book->getShiftedIndex(), 1), {"#", "#", "#"}, '#', Items::paper);
	
	// Beta: Fence recipe (Recipes.java:32)
	addShapedRecipy(ItemStack(Tile::fence.id, 2), {"###", "###"}, '#', Items::stick);
	
	// Beta: Record player recipe (Recipes.java:33)
	addShapedRecipy(ItemStack(Tile::recordPlayer.id, 1), {"###", "#X#", "###"}, '#', TILE_PTR(wood), 'X', Items::diamond);
	
	// Beta: Music block recipe - SKIP (Tile.musicBlock doesn't exist in Alpha)
	// addShapedRecipy(ItemStack(Tile.musicBlock.id, 1), {"###", "#X#", "###"}, '#', TILE_PTR(wood), 'X', Items::redstone);
	
	// Beta: Bookshelf recipe (Recipes.java:35)
	addShapedRecipy(ItemStack(Tile::bookshelf.id, 1), {"###", "XXX", "###"}, '#', TILE_PTR(wood), 'X', Items::book);
	
	// Beta: Snow block recipe (Recipes.java:36)
	addShapedRecipy(ItemStack(Tile::snow.id, 1), {"##", "##"}, '#', Items::snowball);
	
	// Beta: Clay block recipe (Recipes.java:37)
	addShapedRecipy(ItemStack(Tile::clay.id, 1), {"##", "##"}, '#', Items::clay);
	
	// Beta: Red brick recipe (Recipes.java:38)
	addShapedRecipy(ItemStack(Tile::redBrick.id, 1), {"##", "##"}, '#', Items::brick);
	
	// Beta: Light gem recipe (Recipes.java:39)
	addShapedRecipy(ItemStack(Tile::lightGem.id, 1), {"###", "###", "###"}, '#', Items::lightStoneDust);
	
	// Beta: Cloth recipe (Recipes.java:40)
	addShapedRecipy(ItemStack(Tile::cloth.id, 1), {"###", "###", "###"}, '#', Items::silk);
	
	// Beta: TNT recipe (Recipes.java:41)
	addShapedRecipy(ItemStack(Tile::tnt.id, 1), {"X#X", "#X#", "X#X"}, 'X', Items::gunpowder, '#', TILE_PTR(sand));
	
	// Beta: Stone slab recipe (Recipes.java:42)
	addShapedRecipy(ItemStack(Tile::stoneSlabHalf.id, 3), {"###"}, '#', TILE_PTR(cobblestone));
	
	// Beta: Ladder recipe (Recipes.java:43)
	addShapedRecipy(ItemStack(Tile::ladder.id, 1), {"# #", "###", "# #"}, '#', Items::stick);
	
	// Beta: Wood door recipe (Recipes.java:44)
	addShapedRecipy(ItemStack(Items::doorWood->getShiftedIndex(), 1), {"##", "##", "##"}, '#', TILE_PTR(wood));
	
	// Beta: Iron door recipe (Recipes.java:45)
	addShapedRecipy(ItemStack(Items::doorSteel->getShiftedIndex(), 1), {"##", "##", "##"}, '#', Items::ingotIron);
	
	// Beta: Sign recipe (Recipes.java:46)
	addShapedRecipy(ItemStack(Items::sign->getShiftedIndex(), 1), {"###", "###", " X "}, '#', TILE_PTR(wood), 'X', Items::stick);
	
	// Beta: Cake recipe - SKIP (Item.cake doesn't exist in Alpha)
	// addShapedRecipy(ItemStack(Item.cake.id, 1), {"AAA", "BEB", "CCC"}, 'A', Item.milk, 'B', Item.sugar, 'C', Item.wheat, 'E', Item.egg);
	
	// Beta: Sugar recipe - SKIP (Item.sugar doesn't exist in Alpha)
	// addShapedRecipy(ItemStack(Item.sugar.id, 1), {"#"}, '#', Item.reeds);
	
	// Beta: Wood from log recipe (Recipes.java:49)
	addShapedRecipy(ItemStack(Tile::wood.id, 4), {"#"}, '#', TILE_PTR(treeTrunk));
	
	// Beta: Stick recipe (Recipes.java:50)
	addShapedRecipy(ItemStack(Items::stick->getShiftedIndex(), 4), {"#", "#"}, '#', TILE_PTR(wood));
	
	// Beta: Torch recipe - coal (Recipes.java:51)
	addShapedRecipy(ItemStack(Tile::torch.id, 4), {"X", "#"}, 'X', Items::coal, '#', Items::stick);
	
	// Beta: Torch recipe - charcoal (Recipes.java:52)
	addShapedRecipy(ItemStack(Tile::torch.id, 4), {"X", "#"}, 'X', ItemStack(Items::coal->getShiftedIndex(), 1, 1), '#', Items::stick);
	
	// Beta: Bowl recipe (Recipes.java:53)
	addShapedRecipy(ItemStack(Items::bowlEmpty->getShiftedIndex(), 4), {"# #", " # "}, '#', TILE_PTR(wood));
	
	// Beta: Rail recipe (Recipes.java:54)
	addShapedRecipy(ItemStack(Tile::rail.id, 16), {"X X", "X#X", "X X"}, 'X', Items::ingotIron, '#', Items::stick);
	
	// Beta: Minecart recipe (Recipes.java:55)
	addShapedRecipy(ItemStack(Items::minecartEmpty->getShiftedIndex(), 1), {"# #", "###"}, '#', Items::ingotIron);
	
	// Beta: Lit pumpkin recipe (Recipes.java:56)
	addShapedRecipy(ItemStack(Tile::litPumpkin.id, 1), {"A", "B"}, 'A', TILE_PTR(pumpkin), 'B', TILE_PTR(torch));
	
	// Beta: Minecart with chest recipe (Recipes.java:57)
	addShapedRecipy(ItemStack(Items::minecartCrate->getShiftedIndex(), 1), {"A", "B"}, 'A', TILE_PTR(chest), 'B', Items::minecartEmpty);
	
	// Beta: Minecart with furnace recipe (Recipes.java:58)
	addShapedRecipy(ItemStack(Items::minecartPowered->getShiftedIndex(), 1), {"A", "B"}, 'A', TILE_PTR(furnace), 'B', Items::minecartEmpty);
	
	// Beta: Boat recipe (Recipes.java:59)
	addShapedRecipy(ItemStack(Items::boat->getShiftedIndex(), 1), {"# #", "###"}, '#', TILE_PTR(wood));
	
	// Beta: Empty bucket recipe (Recipes.java:60)
	addShapedRecipy(ItemStack(Items::bucketEmpty->getShiftedIndex(), 1), {"# #", " # "}, '#', Items::ingotIron);
	
	// Beta: Flint and steel recipe (Recipes.java:61)
	addShapedRecipy(ItemStack(Items::flintAndSteel->getShiftedIndex(), 1), {"A ", " B"}, 'A', Items::ingotIron, 'B', Items::flint);
	
	// Beta: Bread recipe (Recipes.java:62)
	addShapedRecipy(ItemStack(Items::bread->getShiftedIndex(), 1), {"###"}, '#', Items::wheat);
	
	// Beta: Wood stairs recipe (Recipes.java:63)
	addShapedRecipy(ItemStack(Tile::stairs_wood.id, 4), {"#  ", "## ", "###"}, '#', TILE_PTR(wood));
	
	// Beta: Fishing rod recipe (Recipes.java:64)
	addShapedRecipy(ItemStack(Items::fishingRod->getShiftedIndex(), 1), {"  #", " #X", "# X"}, '#', Items::stick, 'X', Items::silk);
	
	// Beta: Stone stairs recipe (Recipes.java:65)
	addShapedRecipy(ItemStack(Tile::stairs_stone.id, 4), {"#  ", "## ", "###"}, '#', TILE_PTR(cobblestone));
	
	// Beta: Painting recipe (Recipes.java:66)
	addShapedRecipy(ItemStack(Items::painting->getShiftedIndex(), 1), {"###", "#X#", "###"}, '#', Items::stick, 'X', TILE_PTR(cloth));
	
	// Beta: Golden apple recipe (Recipes.java:67)
	addShapedRecipy(ItemStack(Items::appleGold->getShiftedIndex(), 1), {"###", "#X#", "###"}, '#', TILE_PTR(goldBlock), 'X', Items::appleRed);
	
	// Beta: Lever recipe (Recipes.java:68)
	addShapedRecipy(ItemStack(Tile::lever.id, 1), {"X", "#"}, '#', TILE_PTR(cobblestone), 'X', Items::stick);
	
	// Beta: Not gate recipe (Recipes.java:69) - Redstone torch
	addShapedRecipy(ItemStack(Tile::notGate_on.id, 1), {"X", "#"}, '#', Items::stick, 'X', Items::redstone);
	
	// Beta: Clock recipe (Recipes.java:70)
	addShapedRecipy(ItemStack(Items::pocketSundial->getShiftedIndex(), 1), {" # ", "#X#", " # "}, '#', Items::ingotGold, 'X', Items::redstone);
	
	// Beta: Compass recipe (Recipes.java:71)
	addShapedRecipy(ItemStack(Items::compass->getShiftedIndex(), 1), {" # ", "#X#", " # "}, '#', Items::ingotIron, 'X', Items::redstone);
	
	// Beta: Button recipe (Recipes.java:72)
	addShapedRecipy(ItemStack(Tile::button.id, 1), {"#", "#"}, '#', TILE_PTR(rock));
	
	// Beta: Stone pressure plate recipe (Recipes.java:73)
	addShapedRecipy(ItemStack(Tile::pressurePlate_stone.id, 1), {"###"}, '#', TILE_PTR(rock));
	
	// Beta: Wood pressure plate recipe (Recipes.java:74)
	addShapedRecipy(ItemStack(Tile::pressurePlate_wood.id, 1), {"###"}, '#', TILE_PTR(wood));
	
	// Beta: Dispenser recipe - SKIP (Tile.dispenser doesn't exist in Alpha)
	// addShapedRecipy(ItemStack(Tile.dispenser.id, 1), {"###", "#X#", "#R#"}, '#', &Tile::stoneBrick, 'X', Items::bow, 'R', Items::redstone);
}