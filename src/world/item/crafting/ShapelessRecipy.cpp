#include "world/item/crafting/ShapelessRecipy.h"
#include "world/item/crafting/CraftingContainer.h"
#include <algorithm>

// Beta: ShapelessRecipy constructor (ShapelessRecipy.java:12-15)
ShapelessRecipy::ShapelessRecipy(const ItemStack &result, const std::vector<ItemStack> &ingredients)
	: result(result), ingredients(ingredients)
{
}

// Beta: ShapelessRecipy.matches() - checks if recipe matches container (ShapelessRecipy.java:18-43)
bool ShapelessRecipy::matches(CraftingContainer &container)
{
	std::vector<ItemStack> remainingIngredients = ingredients;
	
	for (int_t y = 0; y < 3; ++y)
	{
		for (int_t x = 0; x < 3; ++x)
		{
			ItemStack containerItem = container.getItem(x, y);
			if (containerItem.isEmpty())
				continue;
			
			bool matched = false;
			for (auto it = remainingIngredients.begin(); it != remainingIngredients.end(); ++it)
			{
				if (containerItem.itemID == it->itemID && 
				    (it->itemDamage == -1 || containerItem.itemDamage == it->itemDamage))
				{
					matched = true;
					remainingIngredients.erase(it);
					break;
				}
			}
			
			if (!matched)
				return false;
		}
	}
	
	return remainingIngredients.empty();
}

// Beta: ShapelessRecipy.assemble() - creates result item (ShapelessRecipy.java:46-48)
ItemStack ShapelessRecipy::assemble(CraftingContainer &container)
{
	return ItemStack(result.itemID, result.stackSize, result.itemDamage);
}

// Beta: ShapelessRecipy.size() - returns recipe size (ShapelessRecipy.java:51-53)
int_t ShapelessRecipy::size()
{
	return (int_t)ingredients.size();
}