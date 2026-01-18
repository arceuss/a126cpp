#pragma once

#include "world/level/tile/entity/TileEntity.h"
#include "world/item/ItemStack.h"
#include <vector>
#include <memory>

class Player;

// Beta 1.2 ChestTileEntity
// Reference: newb12/net/minecraft/world/level/tile/entity/ChestTileEntity.java
class ChestTileEntity : public TileEntity
{
private:
	// Beta: ItemInstance[] items = new ItemInstance[36] (ChestTileEntity.java:10)
	std::vector<std::shared_ptr<ItemStack>> items;

public:
	ChestTileEntity();
	
	virtual jstring getEncodeId() const override { return u"Chest"; }
	
	// Beta: Container interface methods (ChestTileEntity.java:13-52)
	int_t getContainerSize() const { return 27; }  // Beta: returns 27 (ChestTileEntity.java:14)
	std::shared_ptr<ItemStack> getItem(int_t slot);
	std::shared_ptr<ItemStack> removeItem(int_t slot, int_t count);
	void setItem(int_t slot, std::shared_ptr<ItemStack> itemStack);
	jstring getName() const { return u"Chest"; }
	int_t getMaxStackSize() const { return 64; }
	
	virtual void load(CompoundTag &tag) override;
	virtual void save(CompoundTag &tag) override;
	
	// Beta: stillValid() - checks if player can access (ChestTileEntity.java:100-102)
	bool stillValid(Player &player);
	
	// Direct setter for world generation (doesn't call setChanged)
	void setItemDirect(int_t slot, std::shared_ptr<ItemStack> itemStack);
};
