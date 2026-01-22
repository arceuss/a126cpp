#include "world/item/ItemSword.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/entity/Mob.h"
#include "world/entity/Entity.h"

ItemSword::ItemSword(int_t id, int_t tier)
	: Item(id)
{
	// Alpha: ItemSword.java:8-14
	this->maxStackSize = 1;  // Swords don't stack
	this->maxDamage = 32 << tier;  // 32 << tier
	if (tier == 3) {
		this->maxDamage *= 4;  // Diamond: 32 << 3 * 4 = 256 * 4 = 1024
	}
	
	// Alpha: weaponDamage = 4 + tier * 2 (ItemSword.java:14)
	this->weaponDamage = 4 + tier * 2;
	
	// Note: isFull3D() and isHandEquipped() are overridden to return true
	// We don't need to set bFull3D here since the overridden methods handle it
}

float ItemSword::getStrVsBlock(ItemStack &stack, Tile &tile)
{
	// Alpha: ItemSword.java:17-19 - swords are slow at breaking blocks
	return 1.5F;
}

void ItemSword::hitEntity(ItemStack &stack, Mob &entity)
{
	// Alpha: ItemSword.java:21-23 - damage item by 1 when hitting entity
	stack.damageItem(1);
}

void ItemSword::hitBlock(ItemStack &stack, int_t blockID, int_t x, int_t y, int_t z)
{
	// Alpha: ItemSword.java:25-27 - damage item by 2 when hitting block
	stack.damageItem(2);
}
