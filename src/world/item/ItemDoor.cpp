#include "world/item/ItemDoor.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/DoorTile.h"
#include "world/level/material/Material.h"
#include "Facing.h"
#include "util/Mth.h"

// Beta: DoorItem(int var1, Material var2) (DoorItem.java:12-17)
// Alpha: ItemDoor(int var1, Material var2) (ItemDoor.java:6-11)
ItemDoor::ItemDoor(int_t id, const Material &material) : Item(id), material(material)
{
	// Beta: this.maxDamage = 64 (DoorItem.java:15)
	setMaxDamage(64);
	// Beta: this.maxStackSize = 1 (DoorItem.java:16)
	setMaxStackSize(1);
}

// Beta: useOn() - places door (DoorItem.java:20-78)
// Alpha: onItemUse() - same logic (ItemDoor.java:13-71)
bool ItemDoor::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Can only place on top face (DoorItem.java:21-23)
	if (face != Facing::UP)
		return false;
	
	// Beta: Move to block above (DoorItem.java:24)
	y++;
	
	// Beta: Determine door type (DoorItem.java:25-30)
	Tile *doorTile = nullptr;
	if (material == Material::wood)
		doorTile = &Tile::door_wood;
	else
		doorTile = &Tile::door_iron;
	
	// Beta: Check if can place (DoorItem.java:32-34)
	if (!doorTile->mayPlace(level, x, y, z))
		return false;
	
	// Beta: Calculate door direction from player rotation (DoorItem.java:35-68)
	int_t direction = Mth::floor((player.yRot + 180.0f) * 4.0f / 360.0f - 0.5f) & 3;
	int_t offsetX = 0;
	int_t offsetZ = 0;
	
	if (direction == 0)
		offsetZ = 1;
	else if (direction == 1)
		offsetX = -1;
	else if (direction == 2)
		offsetZ = -1;
	else if (direction == 3)
		offsetX = 1;
	
	// Beta: Check solid blocks on both sides (DoorItem.java:54-55)
	int_t leftSolid = (level.isSolidTile(x - offsetX, y, z - offsetZ) ? 1 : 0) + 
	                  (level.isSolidTile(x - offsetX, y + 1, z - offsetZ) ? 1 : 0);
	int_t rightSolid = (level.isSolidTile(x + offsetX, y, z + offsetZ) ? 1 : 0) + 
	                   (level.isSolidTile(x + offsetX, y + 1, z + offsetZ) ? 1 : 0);
	
	// Beta: Check if door already exists on either side (DoorItem.java:56-57)
	bool leftHasDoor = level.getTile(x - offsetX, y, z - offsetZ) == doorTile->id || 
	                   level.getTile(x - offsetX, y + 1, z - offsetZ) == doorTile->id;
	bool rightHasDoor = level.getTile(x + offsetX, y, z + offsetZ) == doorTile->id || 
	                    level.getTile(x + offsetX, y + 1, z + offsetZ) == doorTile->id;
	
	// Beta: Determine door opening direction (DoorItem.java:58-63)
	bool openLeft = false;
	if (leftHasDoor && !rightHasDoor)
		openLeft = true;
	else if (rightSolid > leftSolid)
		openLeft = true;
	
	// Beta: Adjust direction if opening left (DoorItem.java:65-68)
	if (openLeft)
	{
		direction = (direction - 1) & 3;
		direction += 4;
	}
	
	// Beta: Place door blocks (DoorItem.java:70-73)
	level.setTile(x, y, z, doorTile->id);
	level.setData(x, y, z, direction);
	level.setTile(x, y + 1, z, doorTile->id);
	level.setData(x, y + 1, z, direction + 8);
	
	// Beta: Decrement stack (DoorItem.java:74)
	stack.stackSize--;
	return true;
}
