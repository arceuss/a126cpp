#pragma once

#include "world/item/Item.h"
#include "java/Type.h"

class ItemStack;
class Player;
class Level;

// Alpha 1.2.6 FoodItem class
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/ItemFood.java
// Note: Alpha uses "FoodItem" but file is ItemFood.java
class ItemFood : public Item
{
protected:
	int_t healAmount;

public:
	ItemFood(int_t id, int_t healAmount);
	virtual ~ItemFood() {}
	
	// Alpha: ItemFood.onItemRightClick() - consumes item and heals player
	ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
