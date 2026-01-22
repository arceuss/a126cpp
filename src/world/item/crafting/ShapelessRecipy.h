#pragma once

#include "world/item/crafting/Recipy.h"
#include "world/item/ItemStack.h"
#include <vector>

class CraftingContainer;

// Beta: ShapelessRecipy class (ShapelessRecipy.java:8-54)
class ShapelessRecipy : public Recipy
{
private:
	ItemStack result;
	std::vector<ItemStack> ingredients;
	
public:
	// Beta: ShapelessRecipy constructor (ShapelessRecipy.java:12-15)
	ShapelessRecipy(const ItemStack &result, const std::vector<ItemStack> &ingredients);
	
	// Beta: Recipy interface implementation
	bool matches(CraftingContainer &container) override;
	ItemStack assemble(CraftingContainer &container) override;
	int_t size() override;
};