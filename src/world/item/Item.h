#pragma once

#include <array>
#include <stdexcept>
#include "java/Type.h"
#include "Facing.h"

class ItemStack;
class Tile;
class Entity;
class Mob;  // Alpha: EntityLiving -> Mob in this codebase
class Player;
class Level;

// Alpha 1.2.6 Item base class
// Reference: apclient/minecraft/src/net/minecraft/src/Item.java
class Item
{
public:
	// Item registry: shiftedIndex = 256 + itemID (Item.java:114)
	static std::array<Item *, 32000> itemsList;
	
protected:
	int_t itemID;  // Base item ID (can be > 255 for records, etc.)
	int_t shiftedIndex;  // Actual ID = 256 + itemID (stored in itemsList[shiftedIndex])
	int_t maxStackSize = 64;
	int_t maxDamage = 32;
	int_t iconIndex = 0;  // Beta: Item.iconIndex - icon index for rendering
	bool bFull3D = false;  // Alpha: Item.bFull3D - whether item renders in 3D (Item.java:109)
	bool hasSubtypes = false;  // Alpha: Item.hasSubtypes (Item.java:110)
	
public:
	Item(int_t id);
	virtual ~Item() {}
	
	int_t getItemID() const { return itemID; }
	int_t getShiftedIndex() const { return shiftedIndex; }
	int_t getMaxStackSize() const { return maxStackSize; }
	int_t getMaxDamage() const { return maxDamage; }
	
	// Block interaction
	virtual float getStrVsBlock(ItemStack &stack, Tile &tile);
	virtual bool canHarvestBlock(Tile &tile);
	
	// Durability consumption
	virtual void hitEntity(ItemStack &stack, Mob &entity);  // Alpha: EntityLiving -> Mob
	virtual void hitBlock(ItemStack &stack, int_t blockID, int_t x, int_t y, int_t z);
	
	// Beta: Item.interactEnemy() - interact with entity (Item.java:185-187)
	virtual void interactEnemy(ItemStack &stack, Mob &entity) {}
	
	// Beta: Item.hurtEnemy() - hurt entity (Item.java:189-191)
	virtual void hurtEnemy(ItemStack &stack, Mob &entity) {}
	
	// Beta: Item.getAttackDamage(Entity var1) - returns attack damage (Item.java:206-208)
	virtual int_t getAttackDamage(Entity &entity) { return 1; }
	
	// Item properties
	Item &setMaxStackSize(int_t size);
	Item &setMaxDamage(int_t damage);
	
	// Beta: Item.setIconIndex() - sets icon index for rendering
	Item &setIconIndex(int_t iconIndex);
	
	// Beta: Item.getIconIndex() - returns icon index for rendering
	virtual int_t getIconIndex(const ItemStack &stack);
	
	// Beta: Item.useOn() - uses item on block (Item.java:162-164)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face);
	
	// Beta: Item.use() - uses item (Item.java:170-172)
	virtual ItemStack use(ItemStack &stack, Level &level, Player &player);
	
	// Beta: Item.getLevelDataForAuxValue() - converts aux value to level data (Item.java:178-180)
	virtual int_t getLevelDataForAuxValue(int_t auxValue) { return 0; }
	
	// Alpha: Item.setFull3D() - sets item to render in 3D (Item.java:168-171)
	Item &setFull3D();
	
	// Alpha: Item.isFull3D() - returns whether item renders in 3D (Item.java:173-175)
	// Made virtual so tool classes can override it (ItemTool.java:45-46, ItemSword.java:33-35)
	virtual bool isFull3D() const { return bFull3D; }
	
	// Alpha: Item.shouldRotateAroundWhenRendering() - for items that need rotation (Item.java:177-179)
	virtual bool shouldRotateAroundWhenRendering() const { return false; }
	
	// Beta: Item.isHandEquipped() - returns true for tools/swords (Beta uses this, Alpha uses isFull3D)
	// Beta: Item.java:222-224 - can be overridden by subclasses
	virtual bool isHandEquipped() const { return bFull3D; }
	
	// Beta: Item.isMirroredArt() - for items that need mirroring (like bows)
	virtual bool isMirroredArt() const { return false; }
};