#pragma once

#include "world/item/Item.h"

// Beta 1.2: SaddleItem.java
// Alpha 1.2.6: ItemSaddle.java
class ItemSaddle : public Item
{
public:
	ItemSaddle(int_t id);
	
	// Beta: interactEnemy() - saddles pig (SaddleItem.java:14-22)
	virtual void interactEnemy(ItemStack &stack, Mob &entity) override;
	
	// Beta: hurtEnemy() - calls interactEnemy (SaddleItem.java:25-27)
	virtual void hurtEnemy(ItemStack &stack, Mob &entity) override;
};
