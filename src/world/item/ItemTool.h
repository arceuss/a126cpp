#pragma once

#include "world/item/Item.h"
#include <vector>

class ItemStack;
class Tile;
class Mob;  // Alpha: EntityLiving -> Mob in this codebase
class Entity;

// Alpha 1.2.6 ItemTool base class
// Reference: apclient/minecraft/src/net/minecraft/src/ItemTool.java
class ItemTool : public Item
{
protected:
	std::vector<int_t> blocksEffectiveAgainst;  // Block IDs this tool is effective against
	float efficiencyOnProperMaterial = 4.0F;
	int_t damageVsEntity = 0;
	int_t ingredientQuality = 0;  // Tool tier (0=wood, 1=stone, 2=iron, 3=diamond)

public:
	ItemTool(int_t id, int_t damageVsEntity, int_t tier, const std::vector<int_t> &effectiveBlocks);
	virtual ~ItemTool() {}
	
	// Block interaction
	float getStrVsBlock(ItemStack &stack, Tile &tile) override;
	
	// Durability consumption
	void hitEntity(ItemStack &stack, Mob &entity) override;  // Alpha: EntityLiving -> Mob
	void hitBlock(ItemStack &stack, int_t blockID, int_t x, int_t y, int_t z) override;
	
	int_t getDamageVsEntity(Entity &entity) const { return damageVsEntity; }
	int_t getIngredientQuality() const { return ingredientQuality; }
	
	// Alpha: ItemTool.isFull3D() returns true (ItemTool.java:45-46)
	bool isFull3D() const override { return true; }
	
	// Beta: DiggerItem.isHandEquipped() returns true (DiggerItem.java:50-51)
	bool isHandEquipped() const override { return true; }
};
