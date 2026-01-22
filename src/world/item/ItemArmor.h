#pragma once

#include "world/item/Item.h"

// Beta 1.2: ArmorItem.java
// Alpha 1.2.6: ItemArmor.java
class ItemArmor : public Item
{
private:
	static const int_t defensePerSlot[4];  // Beta: defensePerSlot (ArmorItem.java:4)
	static const int_t healthPerSlot[4];   // Beta: healthPerSlot (ArmorItem.java:5)

public:
	int_t tier;   // Beta: tier (ArmorItem.java:6), Alpha: armorLevel (ItemArmor.java:6)
	int_t slot;   // Beta: slot (ArmorItem.java:7), Alpha: armorType (ItemArmor.java:7)
	int_t defense;  // Beta: defense (ArmorItem.java:8), Alpha: damageReduceAmmount (ItemArmor.java:8)
	int_t icon;   // Beta: icon (ArmorItem.java:9), Alpha: renderIndex (ItemArmor.java:9)

	ItemArmor(int_t id, int_t tier, int_t iconIndex, int_t slot);
};
