#pragma once

#include "world/item/Item.h"
#include "world/level/material/Material.h"

// Beta 1.2: DoorItem.java
// Alpha 1.2.6: ItemDoor.java
class ItemDoor : public Item
{
private:
	const Material &material;  // Beta: material (DoorItem.java:10), Alpha: field_321_a (ItemDoor.java:4)

public:
	ItemDoor(int_t id, const Material &material);
	
	// Beta: useOn() - places door (DoorItem.java:20-78)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
};
