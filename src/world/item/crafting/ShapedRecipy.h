#pragma once

#include "world/item/crafting/Recipy.h"
#include "world/item/ItemStack.h"
#include <memory>
#include <vector>

class CraftingContainer;

// Beta: ShapedRecipy class (ShapedRecipy.java:6-81)
class ShapedRecipy : public Recipy
{
private:
	int_t width;
	int_t height;
	std::vector<ItemStack> recipeItems;  // Can contain empty ItemStack (itemID=0) for null
	ItemStack result;
	
public:
	int_t resultId;  // Beta: resultId (ShapedRecipy.java:11)
	
	// Beta: ShapedRecipy constructor (ShapedRecipy.java:13-19)
	ShapedRecipy(int_t width, int_t height, const std::vector<ItemStack> &recipeItems, const ItemStack &result);
	
	// Beta: Recipy interface implementation
	bool matches(CraftingContainer &container) override;
	ItemStack assemble(CraftingContainer &container) override;
	int_t size() override;
	
private:
	// Beta: matches() helper method (ShapedRecipy.java:38-70)
	bool matches(CraftingContainer &container, int_t x, int_t y, bool mirror);
};