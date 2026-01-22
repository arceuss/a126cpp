#pragma once

#include "world/item/ItemStack.h"
#include "java/Type.h"

// Beta: CraftingContainer interface for recipe matching (CraftingContainer.java:7-36)
// Simplified version that works with 2x2 crafting grid
class CraftingContainer
{
public:
	virtual ~CraftingContainer() {}
	
	// Beta: getItem(int x, int y) - gets item at position (CraftingContainer.java:29-36)
	// For 2x2 grid: x,y in range 0-1, extended to 0-2 for compatibility with 3x3 recipes
	virtual ItemStack getItem(int_t x, int_t y) = 0;
};