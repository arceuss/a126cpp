#include "world/entity/player/InventoryPlayer.h"
#include "world/entity/player/Player.h"
#include "world/item/ItemArmor.h"
#include "nbt/ListTag.h"
#include "nbt/CompoundTag.h"
#include "util/Memory.h"

#include <stdexcept>

InventoryPlayer::InventoryPlayer(Player *player) : player(player)
{
	// Initialize empty inventory
	for (auto &stack : mainInventory)
	{
		stack = ItemStack();  // Empty stack
	}
	// Alpha 1.2.6: Initialize armorInventory[4] (InventoryPlayer.java:5)
	for (auto &stack : armorInventory)
	{
		stack = ItemStack();  // Empty stack
	}
	carried = ItemStack();  // Beta: Initialize carried item (Inventory.java:26)
}

ItemStack *InventoryPlayer::getCurrentItem()
{
	// Alpha: getCurrentItem() returns mainInventory[currentItem] if 0-8 (InventoryPlayer.java:15-17)
	if (currentItem >= 0 && currentItem < 9)
	{
		ItemStack &stack = mainInventory[currentItem];
		if (!stack.isEmpty())
			return &stack;
	}
	return nullptr;
}

// Alpha: every IInventory accessor maps slots past mainInventory.length into
// armorInventory (InventoryPlayer.java:146-150, 168-172, 227-231)
ItemStack *InventoryPlayer::slotRef(int_t slot)
{
	if (slot < 0 || slot >= getSizeInventory())
		return nullptr;
	if (slot >= (int_t)mainInventory.size())
		return &armorInventory[slot - (int_t)mainInventory.size()];
	return &mainInventory[slot];
}

ItemStack *InventoryPlayer::getStackInSlot(int_t slot)
{
	// Alpha: getStackInSlot(int) (InventoryPlayer.java:225-233)
	ItemStack *stack = slotRef(slot);
	if (stack != nullptr && !stack->isEmpty())
		return stack;
	return nullptr;
}

float InventoryPlayer::getStrVsBlock(Tile &tile)
{
	// Alpha: Delegates to held item's getStrVsBlock (InventoryPlayer.java:262-264)
	ItemStack *held = getCurrentItem();
	if (held != nullptr)
		return held->getStrVsBlock(tile);
	return 1.0F;  // Hand speed
}

bool InventoryPlayer::canHarvestBlock(Tile &tile)
{
	// Alpha: canHarvestBlock checks material.isHarvestable first, then item (InventoryPlayer.java:271-278)
	// If material is harvestable, return true (hand can harvest)
	// If material is not harvestable (e.g., rock/stone), check item's canHarvestBlock
	if (tile.material.getIsHarvestable())
		return true;
	
	// Material not harvestable - check if item can harvest it
	ItemStack *held = getCurrentItem();
	if (held != nullptr && !held->isEmpty())
		return held->canHarvestBlock(tile);
	
	// No tool and material not harvestable - cannot harvest (e.g., stone without pickaxe)
	return false;
}

// Alpha: storeItemStack(ItemStack) - first slot the stack may merge into (InventoryPlayer.java:40-46)
int_t InventoryPlayer::storeItemStack(ItemStack &stack)
{
	for (int_t i = 0; i < (int_t)mainInventory.size(); ++i)
	{
		ItemStack &existing = mainInventory[i];
		// Alpha's single skip condition, split for readability (InventoryPlayer.java:42).
		// isEmpty() stands in for Java's null slot.
		if (existing.isEmpty())
			continue;
		if (existing.itemID != stack.itemID)
			continue;
		if (!existing.isStackable())
			continue;
		if (existing.stackSize >= existing.getMaxStackSize())
			continue;
		if (existing.stackSize >= getInventoryStackLimit())
			continue;
		// Damage only blocks merging for items that use it as a subtype
		if (existing.getHasSubtypes() && existing.itemDamage != stack.itemDamage)
			continue;
		return i;
	}
	return -1;
}

int_t InventoryPlayer::storePartialItemStack(ItemStack &stack)
{
	// Alpha: storePartialItemStack() fills at most ONE slot per call and returns the
	// leftover count; addItemStackToInventory drives the loop (InventoryPlayer.java:79-105)
	int_t itemID = stack.itemID;
	int_t remaining = stack.stackSize;
	int_t slot = storeItemStack(stack);
	if (slot < 0)
		slot = getFreeSlot();  // Alpha: getFirstEmptyStack() (InventoryPlayer.java:48-54)

	if (slot < 0)
		return remaining;

	if (mainInventory[slot].isEmpty())
		mainInventory[slot] = ItemStack(itemID, 0, stack.itemDamage);

	int_t toAdd = remaining;
	if (remaining > mainInventory[slot].getMaxStackSize() - mainInventory[slot].stackSize)
		toAdd = mainInventory[slot].getMaxStackSize() - mainInventory[slot].stackSize;

	if (toAdd > getInventoryStackLimit() - mainInventory[slot].stackSize)
		toAdd = getInventoryStackLimit() - mainInventory[slot].stackSize;

	if (toAdd == 0)
		return remaining;

	remaining -= toAdd;
	mainInventory[slot].stackSize += toAdd;
	mainInventory[slot].popTime = 5;  // Alpha: animationsToGo = 5 (InventoryPlayer.java:103)
	return remaining;
}

// Beta: Inventory.getSlot(int var1) - finds slot with item (Inventory.java:32-40)
int_t InventoryPlayer::getSlot(int_t itemID)
{
	for (int_t i = 0; i < 36; ++i)
	{
		if (!mainInventory[i].isEmpty() && mainInventory[i].itemID == itemID)
		{
			return i;
		}
	}
	return -1;
}

bool InventoryPlayer::consumeInventoryItem(int_t itemID)
{
	// Direct Alpha transliteration: InventoryPlayer.java:114-123.
	const int_t slot = getSlot(itemID);
	if (slot < 0)
		return false;
	if (--mainInventory[slot].stackSize <= 0)
		mainInventory[slot] = ItemStack();
	return true;
}

// Beta: Inventory.getSlotWithRemainingSpace(ItemInstance var1) - finds slot with space for item (Inventory.java:42-55)
int_t InventoryPlayer::getSlotWithRemainingSpace(ItemStack &stack)
{
	for (int_t i = 0; i < 36; ++i)
	{
		ItemStack &existing = mainInventory[i];
		if (!existing.isEmpty()
			&& existing.itemID == stack.itemID
			&& existing.isStackable()
			&& existing.stackSize < existing.getMaxStackSize()
			&& existing.stackSize < 64  // Beta: MAX_INVENTORY_STACK_SIZE = 64 (Inventory.java:15)
			&& existing.itemDamage == stack.itemDamage)  // Beta: (!this.items[var2].isStackedByData() || this.items[var2].getAuxValue() == var1.getAuxValue()) - simplified
		{
			return i;
		}
	}
	return -1;
}

// Beta: Inventory.getFreeSlot() - finds first empty slot (Inventory.java:57-65)
int_t InventoryPlayer::getFreeSlot()
{
	for (int_t i = 0; i < 36; ++i)
	{
		if (mainInventory[i].isEmpty())
		{
			return i;
		}
	}
	return -1;
}

// Beta: Inventory.addResource(ItemInstance var1) - adds to existing stack or creates new one (Inventory.java:108-141)
int_t InventoryPlayer::addResource(ItemStack &stack)
{
	int_t itemID = stack.itemID;
	int_t count = stack.stackSize;
	int_t slot = getSlotWithRemainingSpace(stack);
	if (slot < 0)
	{
		slot = getFreeSlot();
	}
	
	if (slot < 0)
	{
		return count;  // Beta: return var3 (Inventory.java:117)
	}
	
	if (mainInventory[slot].isEmpty())
	{
		// Beta: this.items[var4] = new ItemInstance(var2, 0, var1.getAuxValue()) (Inventory.java:120)
		mainInventory[slot] = ItemStack(itemID, 0, stack.itemDamage);
	}
	
	int_t toAdd = count;
	int_t maxStack = mainInventory[slot].getMaxStackSize();
	if (count > maxStack - mainInventory[slot].stackSize)
	{
		toAdd = maxStack - mainInventory[slot].stackSize;  // Beta: var5 = this.items[var4].getMaxStackSize() - this.items[var4].count (Inventory.java:125)
	}
	
	if (toAdd > 64 - mainInventory[slot].stackSize)  // Beta: MAX_INVENTORY_STACK_SIZE = 64 (Inventory.java:128)
	{
		toAdd = 64 - mainInventory[slot].stackSize;
	}
	
	if (toAdd == 0)
	{
		return count;  // Beta: return var3 (Inventory.java:133)
	}
	
	count -= toAdd;  // Beta: var3 -= var5 (Inventory.java:135)
	mainInventory[slot].stackSize += toAdd;  // Beta: this.items[var4].count += var5 (Inventory.java:136)
	mainInventory[slot].popTime = 5;  // Beta: this.items[var4].popTime = 5 (Inventory.java:137)
	return count;  // Beta: return var3 (Inventory.java:138)
}

// Beta: Inventory.add(ItemInstance var1) - main add method (Inventory.java:170-186)
bool InventoryPlayer::add(ItemStack &stack)
{
	// Beta: if (!var1.isDamaged()) (Inventory.java:171)
	if (!stack.isItemDamaged())
	{
		// Beta: var1.count = this.addResource(var1) (Inventory.java:172)
		stack.stackSize = addResource(stack);
		// Beta: if (var1.count == 0) return true (Inventory.java:173-174)
		if (stack.stackSize == 0)
		{
			return true;
		}
	}
	
	// Beta: int var2 = this.getFreeSlot() (Inventory.java:178)
	int_t slot = getFreeSlot();
	if (slot >= 0)
	{
		// Beta: this.items[var2] = var1 (Inventory.java:180)
		mainInventory[slot] = stack;
		mainInventory[slot].popTime = 5;  // Beta: this.items[var2].popTime = 5 (Inventory.java:181)
		return true;  // Beta: return true (Inventory.java:182)
	}
	else
	{
		return false;  // Beta: return false (Inventory.java:184)
	}
}

bool InventoryPlayer::addItemStackToInventory(ItemStack &stack)
{
	// Alpha: addItemStackToInventory (InventoryPlayer.java:125-142)
	if (stack.isItemDamaged())
	{
		int_t slot = getFreeSlot();  // Alpha: getFirstEmptyStack() (InventoryPlayer.java:128)
		if (slot >= 0)
		{
			mainInventory[slot] = stack.copy();  // Alpha: ItemStack.copyItemStack(itemStack)
			mainInventory[slot].popTime = 5;  // Alpha: animationsToGo = 5 (InventoryPlayer.java:131)
			stack.stackSize = 0;
			return true;
		}
		return false;
	}
	
	// Alpha refreshes the comparison size every iteration, so the loop stops as
	// soon as one iteration makes no progress (InventoryPlayer.java:137-141).
	// At most 36 successful iterations plus one no-progress iteration are
	// possible. Keep that invariant explicit so a future bad loop condition
	// fails instead of hanging the whole game or headless suite.
	int_t prevSize;
	int_t iterations = 0;
	do
	{
		if (iterations++ > static_cast<int_t>(mainInventory.size()))
			throw std::runtime_error("Inventory merge loop did not make bounded progress");
		prevSize = stack.stackSize;
		stack.stackSize = storePartialItemStack(stack);
	}
	while (stack.stackSize > 0 && stack.stackSize < prevSize);
	return stack.stackSize < prevSize;
}

void InventoryPlayer::changeCurrentItem(int_t direction)
{
	// Alpha: changeCurrentItem() - scroll wheel hotbar selection (InventoryPlayer.java:55-65)
	if (direction > 0)
	{
		direction = 1;
	}
	else if (direction < 0)
	{
		direction = -1;
	}
	
	for (currentItem -= direction; currentItem < 0; currentItem += 9)
	{
		// Wrap around
	}
	
	while (currentItem >= 9)
	{
		currentItem -= 9;
	}
}

// Beta: Inventory.tick() - decrements popTime for items (Inventory.java:143-149)
void InventoryPlayer::tick()
{
	for (int_t i = 0; i < 36; ++i)
	{
		// Beta: if (this.items[var1] != null && this.items[var1].popTime > 0) (Inventory.java:144-146)
		if (!mainInventory[i].isEmpty() && mainInventory[i].popTime > 0)
		{
			mainInventory[i].popTime--;  // Beta: this.items[var1].popTime-- (Inventory.java:145)
		}
	}
}

// Beta: Inventory.getSelected() - returns items[selected] (Inventory.java:28-30)
ItemStack *InventoryPlayer::getSelected()
{
	if (currentItem >= 0 && currentItem < 36)
	{
		ItemStack &stack = mainInventory[currentItem];
		if (!stack.isEmpty())
			return &stack;
	}
	return nullptr;
}

// Alpha 1.2.6: armorItemInSlot(int var1) - gets armor from slot (InventoryPlayer.java:280-282)
ItemStack *InventoryPlayer::armorItemInSlot(int_t slot)
{
	if (slot >= 0 && slot < 4)
	{
		ItemStack &stack = armorInventory[slot];
		if (!stack.isEmpty())
			return &stack;
	}
	return nullptr;
}

// Alpha: decrStackSize(int, int) - armor slots follow the main inventory (InventoryPlayer.java:144-164)
ItemStack *InventoryPlayer::removeItem(int_t slot, int_t count)
{
	ItemStack *entry = slotRef(slot);
	if (entry != nullptr && !entry->isEmpty())
	{
		if (entry->stackSize <= count)  // Alpha: if (itemStackArray[n].stackSize <= n2) (InventoryPlayer.java:152)
		{
			ItemStack *result = new ItemStack(entry->itemID, entry->stackSize, entry->itemDamage);
			*entry = ItemStack();  // Alpha: itemStackArray[n] = null (InventoryPlayer.java:154)
			return result;
		}
		else
		{
			// Alpha: ItemStack itemStack = itemStackArray[n].splitStack(n2) (InventoryPlayer.java:157)
			ItemStack *result = new ItemStack(entry->itemID, count, entry->itemDamage);
			entry->stackSize -= count;
			if (entry->stackSize <= 0)  // Alpha: if (itemStackArray[n].stackSize == 0) (InventoryPlayer.java:158)
			{
				*entry = ItemStack();
			}
			return result;
		}
	}
	return nullptr;  // Alpha: return null (InventoryPlayer.java:163)
}

// Alpha: setInventorySlotContents(int, ItemStack) (InventoryPlayer.java:166-174)
void InventoryPlayer::setItem(int_t slot, ItemStack &stack)
{
	ItemStack *entry = slotRef(slot);
	if (entry != nullptr)
	{
		*entry = stack;
	}
}

// Beta: Inventory.getAttackDamage(Entity var1) - gets attack damage from selected item (Inventory.java:302-305)
int_t InventoryPlayer::getAttackDamage(Entity &entity)
{
	ItemStack *selected = getSelected();  // Beta: ItemInstance var2 = this.getItem(this.selected) (Inventory.java:303)
	if (selected != nullptr && !selected->isEmpty())
	{
		// Beta: return var2 != null ? var2.getAttackDamage(var1) : 1 (Inventory.java:304)
		return selected->getAttackDamage(entity);
	}
	return 1;  // Beta: default attack damage = 1 (Inventory.java:304)
}

// Beta: Inventory.getArmorValue() - calculates armor value (Inventory.java:320-338)
// Alpha 1.2.6: InventoryPlayer.getTotalArmorValue() (InventoryPlayer.java:284-306)
int_t InventoryPlayer::getArmorValue()
{
	// Alpha 1.2.6: Armor calculation with durability (InventoryPlayer.java:284-306)
	int_t totalDefense = 0;
	int_t totalRemainingDurability = 0;
	int_t totalMaxDurability = 0;
	
	for (int_t i = 0; i < 4; ++i)  // Alpha: for(int var4 = 0; var4 < this.armorInventory.length; ++var4)
	{
		if (!armorInventory[i].isEmpty())  // Alpha: if(this.armorInventory[var4] != null)
		{
			Item *item = armorInventory[i].getItem();
			if (item != nullptr)
			{
				ItemArmor *armorItem = dynamic_cast<ItemArmor *>(item);
				if (armorItem != nullptr)  // Alpha: if(this.armorInventory[var4].getItem() instanceof ItemArmor)
				{
					int_t maxDamage = armorInventory[i].getMaxDamage();  // Alpha: int var5 = this.armorInventory[var4].getMaxDamage()
					int_t currentDamage = armorInventory[i].itemDamage;  // Alpha: int var6 = this.armorInventory[var4].getItemDamageForDisplay()
					int_t remainingDurability = maxDamage - currentDamage;  // Alpha: int var7 = var5 - var6
					totalRemainingDurability += remainingDurability;  // Alpha: var2 += var7
					totalMaxDurability += maxDamage;  // Alpha: var3 += var5
					totalDefense += armorItem->defense;  // Alpha: int var8 = ((ItemArmor)this.armorInventory[var4].getItem()).damageReduceAmmount; var1 += var8
				}
			}
		}
	}
	
	// Alpha: if(var3 == 0) { return 0; } else { return (var1 - 1) * var2 / var3 + 1; }
	if (totalMaxDurability == 0)
	{
		return 0;
	}
	else
	{
		return (totalDefense - 1) * totalRemainingDurability / totalMaxDurability + 1;
	}
}

// Alpha: damageArmor(int) - kept under the Beta call-site name (InventoryPlayer.java:282-290)
void InventoryPlayer::hurtArmor(int_t damage)
{
	for (int_t i = 0; i < (int_t)armorInventory.size(); ++i)
	{
		if (armorInventory[i].isEmpty())
			continue;
		if (dynamic_cast<ItemArmor *>(armorInventory[i].getItem()) == nullptr)
			continue;
		// Alpha: damageItem(n, this.player); ItemStack::damageItem is already the
		// damageable-guarded two-arg form (ItemStack.java:179-190)
		armorInventory[i].damageItem(damage);
		if (armorInventory[i].stackSize != 0)
			continue;
		// Alpha calls func_1097_a(player) here, which is empty (ItemStack.java:132-133)
		armorInventory[i] = ItemStack();
	}
}

// Beta: Inventory.dropAll() - drops all items when player dies (Inventory.java:352-366)
void InventoryPlayer::dropAll()
{
	if (player == nullptr)
		return;
	
	// Beta: Drop all main inventory items (Inventory.java:353-358)
	for (int_t i = 0; i < 36; ++i)
	{
		if (!mainInventory[i].isEmpty())  // Beta: if (this.items[var1] != null) (Inventory.java:354)
		{
			player->drop(mainInventory[i], true);  // Beta: this.player.drop(this.items[var1], true) (Inventory.java:355)
			mainInventory[i] = ItemStack();  // Beta: this.items[var1] = null (Inventory.java:356)
		}
	}
	
	// Alpha: dropAllItems() also drops every armor piece (InventoryPlayer.java:299-303)
	for (int_t i = 0; i < (int_t)armorInventory.size(); ++i)
	{
		if (!armorInventory[i].isEmpty())
		{
			player->drop(armorInventory[i], true);
			armorInventory[i] = ItemStack();
		}
	}
}

// Beta: Inventory.save() - saves inventory to NBT (Inventory.java:234-254)
// newb12: Inventory.save() saves main inventory and armor (Inventory.java:234-251)
void InventoryPlayer::save(ListTag &tag)
{
	// Beta: Save main inventory (Inventory.java:235-241)
	for (int_t i = 0; i < 36; ++i)
	{
		if (!mainInventory[i].isEmpty())
		{
			std::shared_ptr<CompoundTag> stackTag = Util::make_shared<CompoundTag>();
			stackTag->putByte(u"Slot", (byte_t)i);
			mainInventory[i].save(*stackTag);
			tag.add(stackTag);
		}
	}
	
	// Beta: Save armor slots (Inventory.java:244-250)
	// Alpha: Armor slots use slot numbers 100-103 (100+armorIndex)
	for (int_t i = 0; i < 4; ++i)
	{
		if (!armorInventory[i].isEmpty())
		{
			std::shared_ptr<CompoundTag> stackTag = Util::make_shared<CompoundTag>();
			stackTag->putByte(u"Slot", (byte_t)(i + 100));  // Beta: Slot = var4 + 100 (Inventory.java:247)
			armorInventory[i].save(*stackTag);
			tag.add(stackTag);
		}
	}
}

// Beta: Inventory.load() - loads inventory from NBT (Inventory.java:256-274)
// newb12: Inventory.load() loads main inventory and armor (Inventory.java:256-273)
void InventoryPlayer::load(ListTag &tag)
{
	// Clear inventory first
	for (auto &stack : mainInventory)
	{
		stack = ItemStack();
	}
	// Beta: Clear armor inventory (Inventory.java:258)
	for (auto &stack : armorInventory)
	{
		stack = ItemStack();
	}
	
	// Beta: Load all stacks from NBT (Inventory.java:260-272)
	for (int_t i = 0; i < tag.size(); ++i)
	{
		std::shared_ptr<CompoundTag> stackTag = std::dynamic_pointer_cast<CompoundTag>(tag.get(i));
		if (stackTag != nullptr)
		{
			int_t slot = stackTag->getByte(u"Slot") & 0xFF;
			
			// Beta: Create ItemInstance from NBT (Inventory.java:263)
			ItemStack stack(*stackTag);
			Item *item = stack.getItem();
			
			if (item != nullptr)
			{
				// Beta: Main inventory slots (Inventory.java:265-267)
				if (slot >= 0 && slot < 36)
				{
					mainInventory[slot] = stack;
				}
				
				// Beta: Armor slots (100-103) (Inventory.java:269-271)
				// Alpha: Armor slots use slot numbers 100-103 (100+armorIndex)
				if (slot >= 100 && slot < 104)
				{
					int_t armorIndex = slot - 100;
					armorInventory[armorIndex] = stack;
				}
			}
		}
	}
}