#include "world/item/ItemSaddle.h"

#include "world/entity/animal/Pig.h"
#include "world/item/ItemStack.h"

ItemSaddle::ItemSaddle(int_t id) : Item(id)
{
	maxStackSize = 1;
	maxDamage = 64;
}

void ItemSaddle::interactEnemy(ItemStack &stack, Mob &entity)
{
	// Direct Alpha transliteration: ItemSaddle.java:20-25.
	Pig *pig = dynamic_cast<Pig *>(&entity);
	if (pig != nullptr && !pig->hasSaddle())
	{
		pig->setSaddle(true);
		--stack.stackSize;
	}
}

void ItemSaddle::hurtEnemy(ItemStack &stack, Mob &entity)
{
	interactEnemy(stack, entity);
}
