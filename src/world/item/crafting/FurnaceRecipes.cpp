#include "world/item/crafting/FurnaceRecipes.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/OreTile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/GlassTile.h"
#include "world/level/tile/CobblestoneTile.h"

// Beta: FurnaceRecipes singleton instance (FurnaceRecipes.java:10)
FurnaceRecipes FurnaceRecipes::instance;

// Beta: FurnaceRecipes.getInstance() - singleton access (FurnaceRecipes.java:13-15)
FurnaceRecipes &FurnaceRecipes::getInstance()
{
	return instance;
}

// Alpha 1.2.6: FurnaceRecipes() - constructor (empty, recipes initialized in init())
// Beta: FurnaceRecipes() - constructor initializes recipes (FurnaceRecipes.java:17-28)
// Note: In C++, we defer initialization to init() to ensure Items are initialized first
FurnaceRecipes::FurnaceRecipes()
{
	// Constructor is empty - recipes are initialized in init() after Items::initItems()
}

// Alpha 1.2.6: Initialize recipes (must be called after Items::initItems())
// Beta: FurnaceRecipes() - constructor initializes recipes (FurnaceRecipes.java:17-28)
void FurnaceRecipes::init()
{
	// Alpha: Iron ore -> Iron ingot (TileEntityFurnace.java:175)
	// Beta: Iron ore -> Iron ingot (FurnaceRecipes.java:18)
	addFurnaceRecipy(Tile::oreIron.id, std::make_shared<ItemStack>(Items::ingotIron->getShiftedIndex(), 1));
	
	// Alpha: Gold ore -> Gold ingot (TileEntityFurnace.java:175)
	// Beta: Gold ore -> Gold ingot (FurnaceRecipes.java:19)
	addFurnaceRecipy(Tile::oreGold.id, std::make_shared<ItemStack>(Items::ingotGold->getShiftedIndex(), 1));
	
	// Alpha: Diamond ore -> Diamond (TileEntityFurnace.java:175)
	// Beta: Emerald ore -> Emerald (FurnaceRecipes.java:20)
	// Note: Alpha uses "diamond" instead of "emerald"
	addFurnaceRecipy(Tile::oreDiamond.id, std::make_shared<ItemStack>(Items::diamond->getShiftedIndex(), 1));
	
	// Alpha: Sand -> Glass (TileEntityFurnace.java:175)
	// Beta: Sand -> Glass (FurnaceRecipes.java:21)
	addFurnaceRecipy(Tile::sand.id, std::make_shared<ItemStack>(Tile::glass.id, 1));
	
	// Alpha: Raw pork -> Cooked pork (TileEntityFurnace.java:175)
	// Beta: Raw pork chop -> Cooked pork chop (FurnaceRecipes.java:22)
	addFurnaceRecipy(Items::porkRaw->getShiftedIndex(), std::make_shared<ItemStack>(Items::porkCooked->getShiftedIndex(), 1));
	
	// Alpha: Raw fish -> Cooked fish (TileEntityFurnace.java:175)
	// Beta: Raw fish -> Cooked fish (FurnaceRecipes.java:23)
	addFurnaceRecipy(Items::fishRaw->getShiftedIndex(), std::make_shared<ItemStack>(Items::fishCooked->getShiftedIndex(), 1));
	
	// Alpha: Cobblestone -> Stone (TileEntityFurnace.java:175)
	// Beta: Stone brick -> Rock (FurnaceRecipes.java:24)
	// Note: Alpha uses cobblestone, Beta uses stoneBrick
	addFurnaceRecipy(Tile::cobblestone.id, std::make_shared<ItemStack>(Tile::rock.id, 1));
	
	// Alpha 1.2.6: Clay -> Brick (TileEntityFurnace.java:175)
	// Beta: Clay -> Brick (FurnaceRecipes.java:25)
	addFurnaceRecipy(Items::clay->getShiftedIndex(), std::make_shared<ItemStack>(Items::brick->getShiftedIndex(), 1));
	
	// Note: Alpha 1.2.6 does NOT have:
	// - Cactus -> Dye (Beta only, FurnaceRecipes.java:26)
	// - Tree trunk -> Coal/Charcoal (Beta only, FurnaceRecipes.java:27)
	// These recipes are excluded to match Alpha 1.2.6 behavior
}

// Beta: FurnaceRecipes.addFurnaceRecipy() - adds a recipe (FurnaceRecipes.java:30-32)
void FurnaceRecipes::addFurnaceRecipy(int_t inputId, std::shared_ptr<ItemStack> result)
{
	recipes[inputId] = result;
}

// Beta: FurnaceRecipes.isFurnaceItem() - checks if item can be smelted (FurnaceRecipes.java:34-36)
bool FurnaceRecipes::isFurnaceItem(int_t itemId)
{
	return recipes.find(itemId) != recipes.end();
}

// Beta: FurnaceRecipes.getResult() - gets smelting result (FurnaceRecipes.java:38-40)
std::shared_ptr<ItemStack> FurnaceRecipes::getResult(int_t itemId)
{
	auto it = recipes.find(itemId);
	if (it != recipes.end())
	{
		// Beta: Returns a copy of the result ItemInstance
		return std::make_shared<ItemStack>(*it->second);
	}
	return nullptr;
}
