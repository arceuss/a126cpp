#include "world/item/ItemSign.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "Facing.h"
#include "util/Mth.h"

// Beta: SignItem(int var1) (SignItem.java:10-14)
// Alpha: ItemSign(int var1) (ItemSign.java:10-14)
ItemSign::ItemSign(int_t id) : Item(id)
{
	// Beta: this.maxDamage = 64 (SignItem.java:12)
	setMaxDamage(64);
	// Beta: this.maxStackSize = 1 (SignItem.java:13)
	setMaxStackSize(1);
}

// Beta: useOn() - places sign (SignItem.java:17-61)
// Alpha: onItemUse() - same logic (ItemSign.java:13-61)
bool ItemSign::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Can't place on bottom (SignItem.java:18-19)
	if (face == Facing::DOWN)
		return false;
	
	// Beta: Check if block is solid (SignItem.java:20-21)
	if (!level.getMaterial(x, y, z).isSolid())
		return false;
	
	// Beta: Adjust coordinates based on face (SignItem.java:23-41)
	if (face == Facing::UP)
		y++;
	else if (face == Facing::NORTH)
		z--;
	else if (face == Facing::SOUTH)
		z++;
	else if (face == Facing::WEST)
		x--;
	else if (face == Facing::EAST)
		x++;
	
	// Beta: Check if can place sign (SignItem.java:43-44)
	if (!Tile::sign.mayPlace(level, x, y, z))
		return false;
	
	// Beta: Place sign post or wall sign (SignItem.java:46-50)
	if (face == Facing::UP)
	{
		// Sign post - calculate rotation from player yaw
		int_t data = Mth::floor((player.yRot + 180.0f) * 16.0f / 360.0f + 0.5f) & 15;
		level.setTileAndData(x, y, z, Tile::sign.id, data);
	}
	else
	{
		// Wall sign - use face direction
		level.setTileAndData(x, y, z, Tile::wallSign.id, (int_t)face);
	}
	
	// Beta: Decrement stack (SignItem.java:52)
	stack.stackSize--;
	
	// Beta: Open text edit for sign (SignItem.java:53-56)
	std::shared_ptr<TileEntity> tileEntity = level.getTileEntity(x, y, z);
	if (tileEntity != nullptr)
	{
		std::shared_ptr<SignTileEntity> signEntity = std::dynamic_pointer_cast<SignTileEntity>(tileEntity);
		if (signEntity != nullptr)
		{
			// Beta: player.openTextEdit(signEntity) (SignItem.java:55)
			// Note: openTextEdit is only available on LocalPlayer, so we need to check
			// For now, we'll just call it and let the compiler handle it
			// Actually, Player has openTextEdit as a virtual method that does nothing
			// LocalPlayer overrides it
			player.openTextEdit(signEntity);
		}
	}
	
	return true;
}
