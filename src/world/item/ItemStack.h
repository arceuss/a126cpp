#pragma once

#include "java/Type.h"

class Item;
class Tile;
class Mob;  // Alpha: EntityLiving -> Mob
class Entity;
class Player;
class Level;
class CompoundTag;

enum class Facing;

// Alpha 1.2.6 ItemStack - wrapper for item + count + damage
// Reference: apclient/minecraft/src/net/minecraft/src/ItemStack.java
class ItemStack
{
public:
	int_t stackSize = 0;
	int_t itemID = 0;  // shiftedIndex (256 + base item ID)
	int_t itemDamage = 0;  // Durability damage
	int_t popTime = 0;  // Beta: popTime - animation timer for item pickup (ItemInstance.java)
	
	ItemStack();
	ItemStack(int_t shiftedIndex);
	ItemStack(int_t shiftedIndex, int_t count);
	ItemStack(int_t shiftedIndex, int_t count, int_t damage);
	ItemStack(CompoundTag &tag);  // Alpha: ItemStack(NBTTagCompound var1) - construct from NBT
	
	// Copy and move constructors
	ItemStack(const ItemStack &other) = default;
	ItemStack(ItemStack &&other) noexcept = default;
	ItemStack &operator=(const ItemStack &other) = default;
	ItemStack &operator=(ItemStack &&other) noexcept = default;
	
	Item *getItem() const;
	
	// Block interaction
	float getStrVsBlock(Tile &tile);
	bool canHarvestBlock(Tile &tile);
	
	// Durability consumption (Alpha: hitBlock=1, hitEntity=2)
	void damageItem(int_t amount);
	void hitEntity(Mob &entity);  // Alpha: EntityLiving -> Mob
	void hitBlock(int_t blockID, int_t x, int_t y, int_t z);
	
	// Properties
	int_t getMaxStackSize() const;
	int_t getMaxDamage() const;
	bool isItemStackDamageable() const;
	bool isItemDamaged() const;
	bool isStackable() const;  // Beta: ItemInstance.isStackable() (ItemInstance.java:93-95)
	
	bool isEmpty() const { return stackSize <= 0 || itemID == 0; }
	
	// Beta: ItemStack.getIconIndex() -> getIcon() in ItemRenderer
	int_t getIcon() const;
	
	// Beta: ItemStack.getAuxValue() - returns itemDamage
	int_t getAuxValue() const { return itemDamage; }
	
	// Beta: ItemStack.useOn() - uses item on block (ItemInstance.java:64-66)
	bool useOn(Player &player, Level &level, int_t x, int_t y, int_t z, Facing face);
	
	// Beta: ItemStack.use() - uses item (ItemInstance.java:72-74)
	ItemStack use(Level &level, Player &player);
	
	// Beta: ItemStack.save() and load() - NBT serialization (ItemInstance.java:76-87)
	void save(CompoundTag &tag);
	void load(CompoundTag &tag);
	
	// Beta: ItemStack.copy() - creates a copy (ItemInstance.java:158-160)
	ItemStack copy() const { return ItemStack(itemID, stackSize, itemDamage); }
	
	// Beta: ItemStack.remove() - removes count items and returns new stack (ItemInstance.java:51-54)
	ItemStack remove(int_t count);
	
	// Beta: ItemStack.sameItem() - checks if same item ID and damage (ItemInstance.java:178-180)
	bool sameItem(const ItemStack &other) const;
	
	// Beta: ItemStack.getAttackDamage(Entity var1) - gets attack damage from item (ItemInstance.java:143-145)
	int_t getAttackDamage(Entity &entity);
};
