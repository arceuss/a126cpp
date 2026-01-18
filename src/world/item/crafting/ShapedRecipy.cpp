#include "world/item/crafting/ShapedRecipy.h"
#include "world/item/crafting/CraftingContainer.h"

// Beta: ShapedRecipy constructor (ShapedRecipy.java:13-19)
ShapedRecipy::ShapedRecipy(int_t width, int_t height, const std::vector<ItemStack> &recipeItems, const ItemStack &result)
	: width(width), height(height), recipeItems(recipeItems), result(result)
{
	resultId = result.itemID;
}

// Beta: ShapedRecipy.matches() - checks if recipe matches container (ShapedRecipy.java:22-36)
bool ShapedRecipy::matches(CraftingContainer &container)
{
	for (int_t x = 0; x <= 3 - width; ++x)
	{
		for (int_t y = 0; y <= 3 - height; ++y)
		{
			if (matches(container, x, y, true))
				return true;
			if (matches(container, x, y, false))
				return true;
		}
	}
	return false;
}

// Beta: ShapedRecipy.matches() helper - checks match at position with optional mirroring (ShapedRecipy.java:38-70)
bool ShapedRecipy::matches(CraftingContainer &container, int_t x, int_t y, bool mirror)
{
	for (int_t cx = 0; cx < 3; ++cx)
	{
		for (int_t cy = 0; cy < 3; ++cy)
		{
			int_t rx = cx - x;
			int_t ry = cy - y;
			ItemStack recipeItem;
			
			if (rx >= 0 && ry >= 0 && rx < width && ry < height)
			{
				if (mirror)
				{
					int_t index = (width - rx - 1) + ry * width;
					if (index >= 0 && index < (int_t)recipeItems.size())
						recipeItem = recipeItems[index];
				}
				else
				{
					int_t index = rx + ry * width;
					if (index >= 0 && index < (int_t)recipeItems.size())
						recipeItem = recipeItems[index];
				}
			}
			
			ItemStack containerItem = container.getItem(cx, cy);
			
			// Beta: Check if both are null/empty (ShapedRecipy.java:53)
			if (containerItem.isEmpty() && recipeItem.isEmpty())
				continue;
			
			// Beta: One is null, other isn't - no match (ShapedRecipy.java:54-55)
			if (containerItem.isEmpty() != recipeItem.isEmpty())
				return false;
			
			// Beta: Check item ID match (ShapedRecipy.java:58)
			if (recipeItem.itemID != containerItem.itemID)
				return false;
			
			// Beta: Check aux value match if specified (ShapedRecipy.java:62-63)
			if (recipeItem.itemDamage != -1 && recipeItem.itemDamage != containerItem.itemDamage)
				return false;
		}
	}
	return true;
}

// Beta: ShapedRecipy.assemble() - creates result item (ShapedRecipy.java:73-75)
ItemStack ShapedRecipy::assemble(CraftingContainer &container)
{
	return ItemStack(result.itemID, result.stackSize, result.itemDamage);
}

// Beta: ShapedRecipy.size() - returns recipe size (ShapedRecipy.java:78-80)
int_t ShapedRecipy::size()
{
	return width * height;
}