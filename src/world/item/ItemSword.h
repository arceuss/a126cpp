#pragma once

#include "world/item/Item.h"
#include "java/Type.h"

class ItemStack;
class Tile;
class Mob;  // Alpha: EntityLiving -> Mob in this codebase
class Entity;

// Alpha 1.2.6 SwordItem class
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/ItemSword.java
// Note: Alpha uses "SwordItem" but file is ItemSword.java
class ItemSword : public Item
{
protected:
	int_t weaponDamage;

public:
	ItemSword(int_t id, int_t tier);
	virtual ~ItemSword() {}
	
	// Block interaction
	float getStrVsBlock(ItemStack &stack, Tile &tile) override;
	
	// Durability consumption
	void hitEntity(ItemStack &stack, Mob &entity) override;  // Alpha: EntityLiving -> Mob
	void hitBlock(ItemStack &stack, int_t blockID, int_t x, int_t y, int_t z) override;
	
	int_t getDamageVsEntity(Entity &entity) const { return weaponDamage; }
	int_t getAttackDamage(Entity &entity) override { return weaponDamage; }  // Beta: WeaponItem.getAttackDamage() returns damage
	
	// Alpha: ItemSword.isFull3D() returns true (ItemSword.java:33-35)
	bool isFull3D() const override { return true; }
	
	// Beta: WeaponItem.isHandEquipped() returns true (WeaponItem.java:38-39)
	bool isHandEquipped() const override { return true; }
};
