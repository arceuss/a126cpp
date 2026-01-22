#include "world/item/ItemFlintAndSteel.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FireTile.h"
#include "Facing.h"
#include "java/Random.h"

// Beta: FlintAndSteelItem(int var1) (FlintAndSteelItem.java:8-12)
// Alpha: ItemFlintAndSteel(int var1) (ItemFlintAndSteel.java:4-8)
ItemFlintAndSteel::ItemFlintAndSteel(int_t id) : Item(id)
{
	// Beta: this.maxStackSize = 1 (FlintAndSteelItem.java:10)
	setMaxStackSize(1);
	// Alpha: this.maxDamage = 64 (ItemFlintAndSteel.java:7)
	// Beta: this.maxDamage = 64 (FlintAndSteelItem.java:11)
	setMaxDamage(64);
}

// Beta: useOn() - lights fire on block (FlintAndSteelItem.java:15-48)
// Alpha: onItemUse() - same logic (ItemFlintAndSteel.java:10-46)
bool ItemFlintAndSteel::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Adjust coordinates based on face (FlintAndSteelItem.java:16-38)
	if (face == Facing::DOWN)
		y--;
	else if (face == Facing::UP)
		y++;
	else if (face == Facing::NORTH)
		z--;
	else if (face == Facing::SOUTH)
		z++;
	else if (face == Facing::WEST)
		x--;
	else if (face == Facing::EAST)
		x++;
	
	// Beta: Check if target block is air (FlintAndSteelItem.java:40-44)
	int_t tileId = level.getTile(x, y, z);
	if (tileId == 0)  // Air
	{
		// Beta: Play sound and place fire (FlintAndSteelItem.java:42-43)
		level.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, u"fire.ignite", 1.0f, level.random.nextFloat() * 0.4f + 0.8f);
		level.setTile(x, y, z, Tile::fire.id);
	}
	
	// Beta: Damage item by 1 (FlintAndSteelItem.java:46)
	// Alpha: var1.damageItem(1) (ItemFlintAndSteel.java:44)
	stack.damageItem(1);
	return true;
}
