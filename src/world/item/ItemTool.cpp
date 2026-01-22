#include "world/item/ItemTool.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/entity/Mob.h"
#include "world/entity/Entity.h"

ItemTool::ItemTool(int_t id, int_t damageVsEntity, int_t tier, const std::vector<int_t> &effectiveBlocks)
	: Item(id)
{
	this->ingredientQuality = tier;
	this->blocksEffectiveAgainst = effectiveBlocks;
	this->maxStackSize = 1;  // Tools don't stack (ItemTool.java:13)
	
	// Alpha: maxDamage = 32 << tier, tier 3 (diamond) gets 4x multiplier (ItemTool.java:14-17)
	this->maxDamage = 32 << tier;
	if (tier == 3) {
		this->maxDamage *= 4;  // Diamond: 32 << 3 * 4 = 256 * 4 = 1024
	}
	
	// Alpha: efficiency = (tier + 1) * 2 (ItemTool.java:19)
	this->efficiencyOnProperMaterial = (float)((tier + 1) * 2);
	this->damageVsEntity = damageVsEntity + tier;
	
	// Note: isFull3D() and isHandEquipped() are overridden to return true
	// We don't need to set bFull3D here since the overridden methods handle it
}

float ItemTool::getStrVsBlock(ItemStack &stack, Tile &tile)
{
	// Alpha: Check if block is in effective list (ItemTool.java:23-31)
	for (int_t blockId : blocksEffectiveAgainst)
	{
		if (tile.id == blockId)
		{
			return efficiencyOnProperMaterial;
		}
	}
	
	// Not effective: return 1.0F (hand speed)
	return 1.0F;
}

void ItemTool::hitEntity(ItemStack &stack, Mob &entity)
{
	// Alpha: hitEntity damages item by 2 (ItemTool.java:33-35)
	stack.damageItem(2);
}

void ItemTool::hitBlock(ItemStack &stack, int_t blockID, int_t x, int_t y, int_t z)
{
	// Alpha: hitBlock damages item by 1 (ItemTool.java:37-39)
	stack.damageItem(1);
}
