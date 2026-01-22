#pragma once

#include "java/Type.h"
#include <unordered_map>
#include <memory>

class ItemStack;

// Beta 1.2: FurnaceRecipes.java - smelting recipes for furnace
class FurnaceRecipes
{
private:
	static FurnaceRecipes instance;
	std::unordered_map<int_t, std::shared_ptr<ItemStack>> recipes;
	
	FurnaceRecipes();  // Private constructor for singleton
	
public:
	// Beta: FurnaceRecipes.getInstance() - singleton access (FurnaceRecipes.java:13-15)
	static FurnaceRecipes &getInstance();
	
	// Initialize recipes (must be called after Items::initItems())
	void init();
	
	// Beta: FurnaceRecipes.addFurnaceRecipy() - adds a recipe (FurnaceRecipes.java:30-32)
	void addFurnaceRecipy(int_t inputId, std::shared_ptr<ItemStack> result);
	
	// Beta: FurnaceRecipes.isFurnaceItem() - checks if item can be smelted (FurnaceRecipes.java:34-36)
	bool isFurnaceItem(int_t itemId);
	
	// Beta: FurnaceRecipes.getResult() - gets smelting result (FurnaceRecipes.java:38-40)
	std::shared_ptr<ItemStack> getResult(int_t itemId);
};
