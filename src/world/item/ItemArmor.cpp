#include "world/item/ItemArmor.h"

// Beta: defensePerSlot = {3, 8, 6, 3} (ArmorItem.java:4)
// Alpha: damageReduceAmmountArray = {3, 8, 6, 3} (ItemArmor.java:4)
const int_t ItemArmor::defensePerSlot[4] = {3, 8, 6, 3};

// Beta: healthPerSlot = {11, 16, 15, 13} (ArmorItem.java:5)
// Alpha: maxDamageArray = {11, 16, 15, 13} (ItemArmor.java:5)
const int_t ItemArmor::healthPerSlot[4] = {11, 16, 15, 13};

// Beta: ArmorItem(int var1, int var2, int var3, int var4) (ArmorItem.java:11-19)
// Alpha: ItemArmor(int var1, int var2, int var3, int var4) (ItemArmor.java:11-19)
ItemArmor::ItemArmor(int_t id, int_t tier, int_t iconIndex, int_t slot) : Item(id), tier(tier), slot(slot), icon(iconIndex)
{
	// Beta: this.slot = var4 (ArmorItem.java:14)
	// Beta: this.icon = var3 (ArmorItem.java:15)
	// Beta: this.defense = defensePerSlot[var4] (ArmorItem.java:16)
	this->defense = defensePerSlot[slot];
	
	// Beta: this.maxDamage = healthPerSlot[var4] * 3 << var2 (ArmorItem.java:17)
	// Alpha: this.maxDamage = maxDamageArray[var4] * 3 << var2 (ItemArmor.java:17)
	setMaxDamage(healthPerSlot[slot] * 3 << tier);
	
	// Beta: this.maxStackSize = 1 (ArmorItem.java:18)
	setMaxStackSize(1);
	
	// Beta: Set icon index
	setIconIndex(iconIndex);
}
