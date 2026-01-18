#pragma once

#include "world/item/crafting/Recipy.h"
#include "world/item/crafting/CraftingContainer.h"
#include "world/item/ItemStack.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

class Item;
class Tile;

// Beta: Recipes singleton class (Recipes.java:13-174)
class Recipes
{
public:
	static constexpr int_t ANY_AUX_VALUE = -1;  // Beta: ANY_AUX_VALUE = -1 (Recipes.java:14)
	
	// Beta: getInstance() - singleton access (Recipes.java:18-20)
	static Recipes &getInstance();
	
	// Beta: getItemFor() - finds matching recipe (Recipes.java:164-173)
	ItemStack getItemFor(CraftingContainer &container);
	
	// Beta: addShapedRecipy() - adds shaped recipe (Recipes.java:92-142)
	// Simplified version using helper methods for C++ (since no varargs)
	void addShapedRecipy(const ItemStack &result, const std::vector<std::string> &pattern, 
	                     const std::vector<std::pair<char, ItemStack>> &mappings);
	
	// Beta: addShapelessRecipy() - adds shapeless recipe (Recipes.java:144-162)
	void addShapelessRecipy(const ItemStack &result, const std::vector<ItemStack> &ingredients);
	
	// Helper method for easier recipe addition
	void addShapedRecipyFromStrings(const ItemStack &result, const std::vector<const char*> &pattern,
	                                const std::vector<std::pair<char, ItemStack>> &mappings);
	
	// Helper methods for recipe helper classes - varargs-style support for Item*/Tile*/ItemStack
	// Beta: addShapedRecipy() varargs support - accepts Item, Tile, or ItemStack (Recipes.java:92-142)
	// These methods convert Item*/Tile*/ItemStack to ItemStack and call the main addShapedRecipy method
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key, Item *item);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key, Tile *tile);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key, const ItemStack &stack);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Item *item2);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Tile *tile2);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Tile *tile2);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, const ItemStack &stack1, char key2, Item *item2);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, const ItemStack &stack1, char key2, Tile *tile2);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Tile *tile2, char key3, Item *item3);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Tile *tile1, char key2, Item *item2, char key3, Item *item3);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Tile *tile2, char key3, Item *item3);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Tile *tile3);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3, char key4, Item *item4);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3, char key4, Tile *tile4);
	void addShapedRecipy(const ItemStack &result, const std::vector<const char*> &pattern, char key1, Item *item1, char key2, Item *item2, char key3, Item *item3, char key4, Item *item4, char key5, Item *item5);
	
private:
	std::vector<std::unique_ptr<Recipy>> recipies;  // Beta: List<Recipy> recipies (Recipes.java:16)
	
	Recipes();  // Beta: private constructor (Recipes.java:22)
	
	// Helper methods for recipe initialization
	void initRecipes();  // Initializes all recipes in the same order as Java
};