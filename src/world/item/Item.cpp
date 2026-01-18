#include "world/item/Item.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "Facing.h"
#include "world/entity/Mob.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include <stdexcept>

// Item registry (Item.java:7) - initialized to nullptrs
std::array<Item *, 32000> Item::itemsList = {};

Item::Item(int_t id)
{
	// Beta: Item(int var1) - no range check, just calculates shiftedIndex = 256 + var1
	// Alpha: Item(int var1) - same system
	this->itemID = id;
	this->shiftedIndex = 256 + id;  // Alpha/Beta: shiftedIndex = 256 + itemID
	
	// Beta: Check for conflicts (Item.java:132-134)
	if (itemsList[shiftedIndex] != nullptr)
	{
		// Beta: Just prints "CONFLICT @ " + var1, but we'll throw for safety
		throw std::runtime_error("Item slot " + std::to_string(shiftedIndex) + " is already occupied (id=" + std::to_string(id) + ")");
	}
	
	itemsList[shiftedIndex] = this;
}

float Item::getStrVsBlock(ItemStack &stack, Tile &tile)
{
	// Alpha: Default speed = 1.0F (Item.java:135-137)
	return 1.0F;
}

bool Item::canHarvestBlock(Tile &tile)
{
	// Alpha: Default = false (Item.java:161-163)
	return false;
}

void Item::hitEntity(ItemStack &stack, Mob &entity)
{
	// Alpha: Default = no-op (Item.java:151-152)
}

void Item::hitBlock(ItemStack &stack, int_t blockID, int_t x, int_t y, int_t z)
{
	// Alpha: Default = no-op (Item.java:154-155)
}

Item &Item::setMaxStackSize(int_t size)
{
	maxStackSize = size;
	return *this;
}

Item &Item::setMaxDamage(int_t damage)
{
	maxDamage = damage;
	return *this;
}

Item &Item::setIconIndex(int_t iconIndex)
{
	this->iconIndex = iconIndex;
	return *this;
}

int_t Item::getIconIndex(const ItemStack &stack)
{
	// Beta: Default implementation returns iconIndex
	// Subclasses can override for different icons based on damage/metadata
	return iconIndex;
}

// Beta: Item.useOn() - uses item on block (Item.java:162-164)
bool Item::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Default implementation returns false (Item.java:163)
	return false;
}

// Beta: Item.use() - uses item (Item.java:170-172)
ItemStack Item::use(ItemStack &stack, Level &level, Player &player)
{
	// Beta: Default implementation returns stack unchanged (Item.java:171)
	return stack;
}

// Alpha: Item.setFull3D() - sets item to render in 3D (Item.java:168-171)
Item &Item::setFull3D()
{
	this->bFull3D = true;
	return *this;
}
