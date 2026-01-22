#pragma once

#include "world/item/ItemStack.h"
#include "java/String.h"
#include "java/Type.h"
#include <vector>
#include <memory>

// Alpha 1.2.6 InventoryBasic - simple inventory container for chests
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/InventoryBasic.java
class InventoryBasic
{
private:
	jstring inventoryTitle;
	int_t slotsCount;
	std::vector<std::shared_ptr<ItemStack>> inventoryContents;

public:
	InventoryBasic(jstring title, int_t slots);
	
	// Alpha: getStackInSlot(int var1) (InventoryBasic.java:17-19)
	std::shared_ptr<ItemStack> getStackInSlot(int_t slot);
	
	// Alpha: setInventorySlotContents(int var1, ItemStack var2) (InventoryBasic.java:43-50)
	void setInventorySlotContents(int_t slot, std::shared_ptr<ItemStack> stack);
	
	// Alpha: getSizeInventory() (InventoryBasic.java:52-54)
	int_t getSizeInventory() const { return slotsCount; }
	
	// Alpha: getInvName() (InventoryBasic.java:56-58)
	jstring getInvName() const { return inventoryTitle; }
	
	// Alpha: getInventoryStackLimit() (InventoryBasic.java:60-62)
	int_t getInventoryStackLimit() const { return 64; }
};
