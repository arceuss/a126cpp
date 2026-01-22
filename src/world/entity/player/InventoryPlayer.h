#pragma once

#include <array>
#include <memory>
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "java/Type.h"

class Player;
class Entity;
class ListTag;

// Minimal inventory stub for tool system - full implementation later
// Alpha reference: apclient/minecraft/src/net/minecraft/src/InventoryPlayer.java
class InventoryPlayer
{
public:
	// Alpha: mainInventory[36], currentItem[0-8] (InventoryPlayer.java:4-6)
	std::array<ItemStack, 36> mainInventory;
	// Alpha 1.2.6: armorInventory[4] for armor slots (InventoryPlayer.java:5)
	std::array<ItemStack, 4> armorInventory;
	int_t currentItem = 0;  // Hotbar slot (0-8)
	
	// Beta: Inventory.carried - item being carried in GUI (Inventory.java:26)
	ItemStack carried;  // Beta: ItemInstance carried (Inventory.java:26)
	
	Player *player = nullptr;
	
	InventoryPlayer(Player *player);
	
	// Get currently held item (from hotbar slot 0-8)
	ItemStack *getCurrentItem();
	ItemStack *getStackInSlot(int_t slot);
	
	// Block interaction (delegates to held item)
	float getStrVsBlock(Tile &tile);
	bool canHarvestBlock(Tile &tile);
	
	// Item management
	bool add(ItemStack &stack);  // Beta: Inventory.add(ItemInstance var1) (Inventory.java:170-186)
	bool addItemStackToInventory(ItemStack &stack);  // Alpha-style method (kept for compatibility)
	
	// Alpha: changeCurrentItem() for scroll wheel (InventoryPlayer.java:55-65)
	void changeCurrentItem(int_t direction);
	
	// Beta: Inventory methods (Inventory.java:143-149, 188-212, 28-30, 352-366, 302-305, 307-314, 320-338, 340-349)
	void tick();  // Beta: tick() - decrements popTime for items (Inventory.java:143-149)
	ItemStack *getSelected();  // Beta: getSelected() - returns items[selected] (Inventory.java:28-30)
	ItemStack *removeItem(int_t slot, int_t count);  // Beta: removeItem(int var1, int var2) (Inventory.java:188-212)
	void dropAll();  // Beta: dropAll() - drops all items (Inventory.java:352-366)
	void setItem(int_t slot, ItemStack &stack);  // Beta: setItem(int var1, ItemInstance var2) - sets item at slot (Inventory.java:214-223)
	int_t getAttackDamage(Entity &entity);  // Beta: getAttackDamage(Entity var1) (Inventory.java:302-305)
	int_t getArmorValue();  // Beta: getArmorValue() (Inventory.java:320-338)
	void hurtArmor(int_t damage);  // Beta: hurtArmor(int var1) (Inventory.java:340-349)
	
	// Alpha 1.2.6: armorItemInSlot(int var1) - gets armor from slot (InventoryPlayer.java:280-282)
	ItemStack *armorItemInSlot(int_t slot);
	
	// Beta: Inventory.save() and load() - NBT serialization (Inventory.java:234-274)
	void save(ListTag &tag);
	void load(ListTag &tag);
	
	// Beta: Inventory.getCarried() and setCarried() - carried item in GUI (Inventory.java:411-418)
	ItemStack *getCarried() { return carried.isEmpty() ? nullptr : &carried; }
	void setCarried(const ItemStack &stack) { carried = stack; }
	void setCarried(ItemStack &&stack) { carried = std::move(stack); }
	void setCarriedNull() { carried = ItemStack(); }
	
private:
	// Alpha: storePartialItemStack() - helper for merging stacks (InventoryPlayer.java:103-128)
	int_t storePartialItemStack(ItemStack &stack);
	
	// Beta: Inventory helper methods (Inventory.java:32-65, 108-141)
	int_t getSlot(int_t itemID);  // Beta: getSlot(int var1) - finds slot with item (Inventory.java:32-40)
	int_t getSlotWithRemainingSpace(ItemStack &stack);  // Beta: getSlotWithRemainingSpace(ItemInstance var1) - finds slot with space for item (Inventory.java:42-55)
	int_t getFreeSlot();  // Beta: getFreeSlot() - finds first empty slot (Inventory.java:57-65)
	int_t addResource(ItemStack &stack);  // Beta: addResource(ItemInstance var1) - adds to existing stack or creates new one (Inventory.java:108-141)
};
