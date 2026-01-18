#include "world/item/ItemHoe.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/material/Material.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/Items.h"
#include "Facing.h"
#include "java/Random.h"

// Beta: HoeItem(int var1, Item.Tier var2) (HoeItem.java:10-14)
// Alpha: ItemHoe(int var1, int var2) (ItemHoe.java:4-8)
ItemHoe::ItemHoe(int_t id, int_t tier) : Item(id), tier(tier)
{
	// Beta: this.maxStackSize = 1 (HoeItem.java:12)
	setMaxStackSize(1);
	// Alpha: this.maxDamage = 32 << var2 (ItemHoe.java:7)
	// Beta: this.maxDamage = var2.getUses() (HoeItem.java:13)
	// Alpha durability calculation: 32 << tier
	// For diamond (tier 3): (32 << 3) * 4 = 1024
	if (tier == 3)
		setMaxDamage((32 << 3) * 4);  // Diamond gets 4x multiplier
	else
		setMaxDamage(32 << tier);
}

// Beta: useOn() - tills dirt/grass into farmland (HoeItem.java:17-54)
// Alpha: onItemUse() - same logic (ItemHoe.java:10-40)
bool ItemHoe::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Get tile and material above (HoeItem.java:18-19)
	int_t tileId = level.getTile(x, y, z);
	const Material &materialAbove = level.getMaterial(x, y + 1, z);
	
	// Beta: Check if can till (HoeItem.java:20-21)
	// Can only till grass or dirt, and nothing solid above
	if ((materialAbove.isSolid() || tileId != Tile::grass.id) && tileId != Tile::dirt.id)
		return false;
	
	// Beta: Play sound (HoeItem.java:24-31)
	Tile &farmland = Tile::farmland;
	level.playSound(
		(double)x + 0.5,
		(double)y + 0.5,
		(double)z + 0.5,
		farmland.getSoundType()->getStepSound(),
		(farmland.getSoundType()->getVolume() + 1.0f) / 2.0f,
		farmland.getSoundType()->getPitch() * 0.8f
	);
	
	// Beta: If online, just return true (HoeItem.java:32-34)
	if (level.isOnline)
		return true;
	
	// Beta: Set tile to farmland (HoeItem.java:35)
	level.setTile(x, y, z, farmland.id);
	
	// Beta: Damage item by 1 (HoeItem.java:36)
	stack.damageItem(1);
	
	// Beta: 1/8 chance to drop seeds when tilling grass (HoeItem.java:37-49)
	if (level.random.nextInt(8) == 0 && tileId == Tile::grass.id)
	{
		// Beta: Spawn 1 seed (HoeItem.java:38-48)
		float spread = 0.7f;
		float offsetX = level.random.nextFloat() * spread + (1.0f - spread) * 0.5f;
		float offsetY = 1.2f;
		float offsetZ = level.random.nextFloat() * spread + (1.0f - spread) * 0.5f;
		
		// TODO: Create ItemStack and EntityItem for seeds
		// For now, this is a placeholder - need Items::seeds to be available
		// ItemStack seedStack(Items::seeds->getShiftedIndex(), 1);
		// auto seedEntity = std::make_shared<EntityItem>(level, (double)x + offsetX, (double)y + offsetY, (double)z + offsetZ, seedStack);
		// level.addEntity(seedEntity);
	}
	
	return true;
}
