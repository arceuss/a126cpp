#pragma once

#include "world/item/Item.h"

// Beta 1.2: FishingRodItem.java
// Alpha 1.2.6: ItemFishingRod.java
class ItemFishingRod : public Item
{
public:
	ItemFishingRod(int_t id);
	
	// Beta: isHandEquipped() - returns true (FishingRodItem.java:14-16)
	virtual bool isHandEquipped() const override { return true; }
	
	// Beta: isMirroredArt() - returns true (FishingRodItem.java:19-21)
	virtual bool isMirroredArt() const override { return true; }
	
	// Alpha: isFull3D() - returns true (ItemFishingRod.java:9-11)
	virtual bool isFull3D() const override { return true; }
	
	// Alpha: shouldRotateAroundWhenRendering() - returns true (ItemFishingRod.java:13-15)
	virtual bool shouldRotateAroundWhenRendering() const override { return true; }
	
	// Beta: use() - casts or retrieves fishing hook (FishingRodItem.java:24-39)
	virtual ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
