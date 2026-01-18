#include "world/level/tile/entity/InventoryBasic.h"

InventoryBasic::InventoryBasic(jstring title, int_t slots)
	: inventoryTitle(title)
	, slotsCount(slots)
{
	// Alpha: this.inventoryContents = new ItemStack[var2] (InventoryBasic.java:14)
	inventoryContents.resize(slots, nullptr);
}

std::shared_ptr<ItemStack> InventoryBasic::getStackInSlot(int_t slot)
{
	// Alpha: return this.inventoryContents[var1] (InventoryBasic.java:18)
	if (slot >= 0 && slot < static_cast<int_t>(inventoryContents.size()))
	{
		return inventoryContents[slot];
	}
	return nullptr;
}

void InventoryBasic::setInventorySlotContents(int_t slot, std::shared_ptr<ItemStack> stack)
{
	// Alpha: this.inventoryContents[var1] = var2 (InventoryBasic.java:44)
	if (slot >= 0 && slot < static_cast<int_t>(inventoryContents.size()))
	{
		inventoryContents[slot] = stack;
		
		// Alpha: if(var2 != null && var2.stackSize > this.getInventoryStackLimit()) { var2.stackSize = this.getInventoryStackLimit(); } (InventoryBasic.java:45-47)
		if (stack != nullptr && stack->stackSize > getInventoryStackLimit())
		{
			stack->stackSize = getInventoryStackLimit();
		}
	}
}
