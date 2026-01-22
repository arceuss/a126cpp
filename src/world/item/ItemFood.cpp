#include "world/item/ItemFood.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"

ItemFood::ItemFood(int_t id, int_t healAmount)
	: Item(id)
{
	// Alpha: FoodItem.java:6-10 (a126complete uses FoodItem class name)
	this->healAmount = healAmount;
	this->maxStackSize = 1;  // Food items don't stack in Alpha
}

ItemStack ItemFood::use(ItemStack &stack, Level &level, Player &player)
{
	// Alpha: FoodItem.java:12-16 (a126complete: onItemRightClick)
	// Decrement stack size and heal player
	if (stack.stackSize > 0) {
		stack.stackSize--;
		player.heal(healAmount);
	}
	
	// Return the modified stack (or empty if consumed)
	return stack;
}
