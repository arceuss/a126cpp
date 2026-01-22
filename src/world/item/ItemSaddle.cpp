#include "world/item/ItemSaddle.h"
#include "world/item/ItemStack.h"
#include "world/entity/Mob.h"
// TODO: Include Pig entity when available
// #include "world/entity/animal/Pig.h"

// Beta: SaddleItem(int var1) (SaddleItem.java:7-11)
// Alpha: ItemSaddle(int var1) (ItemSaddle.java:7-11)
ItemSaddle::ItemSaddle(int_t id) : Item(id)
{
	// Beta: this.maxStackSize = 1 (SaddleItem.java:9)
	setMaxStackSize(1);
	// Beta: this.maxDamage = 64 (SaddleItem.java:10)
	setMaxDamage(64);
}

// Beta: interactEnemy() - saddles pig (SaddleItem.java:14-22)
// Alpha: interactWithEntity() - same logic (ItemSaddle.java:13-21)
void ItemSaddle::interactEnemy(ItemStack &stack, Mob &entity)
{
	// Beta: Check if entity is pig (SaddleItem.java:15-21)
	// TODO: Check if entity is Pig and saddle it
	// if (entity instanceof Pig)
	// {
	//     Pig *pig = (Pig *)&entity;
	//     if (!pig->hasSaddle())
	//     {
	//         pig->setSaddle(true);
	//         stack.stackSize--;
	//     }
	// }
}

// Beta: hurtEnemy() - calls interactEnemy (SaddleItem.java:25-27)
// Alpha: hitEntity() - same logic (ItemSaddle.java:23-25)
void ItemSaddle::hurtEnemy(ItemStack &stack, Mob &entity)
{
	interactEnemy(stack, entity);
}
