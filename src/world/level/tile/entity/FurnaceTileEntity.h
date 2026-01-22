#pragma once

#include "world/level/tile/entity/TileEntity.h"
#include "world/item/ItemStack.h"
#include <memory>
#include <vector>

class Player;

// Beta 1.2 FurnaceTileEntity
// Reference: newb12/net/minecraft/world/level/tile/entity/FurnaceTileEntity.java
class FurnaceTileEntity : public TileEntity
{
private:
	// Beta: BURN_INTERVAL = 200 (FurnaceTileEntity.java:15)
	static constexpr int_t BURN_INTERVAL = 200;
	
	// Beta: ItemInstance[] items = new ItemInstance[3] (FurnaceTileEntity.java:16)
	std::vector<std::shared_ptr<ItemStack>> items;
	
public:
	// Beta: litTime, litDuration, tickCount (FurnaceTileEntity.java:17-19)
	int_t litTime = 0;
	int_t litDuration = 0;
	int_t tickCount = 0;

public:
	FurnaceTileEntity();
	
	virtual jstring getEncodeId() const override { return u"Furnace"; }
	
	// Beta: Container interface methods (FurnaceTileEntity.java:22-57)
	int_t getContainerSize() const { return static_cast<int_t>(items.size()); }
	std::shared_ptr<ItemStack> getItem(int_t slot);
	std::shared_ptr<ItemStack> removeItem(int_t slot, int_t count);
	void setItem(int_t slot, std::shared_ptr<ItemStack> itemStack);
	jstring getName() const { return u"Furnace"; }
	int_t getMaxStackSize() const { return 64; }
	
	// Beta: FurnaceTileEntity methods (FurnaceTileEntity.java:107-122)
	int_t getBurnProgress(int_t scale) const;
	int_t getLitProgress(int_t scale);
	bool isLit() const { return litTime > 0; }
	
	// Alpha 1.2.6: Update progress bar values from Packet105UpdateProgressbar
	// Java: ContainerFurnace.func_20112_a(int var1, int var2) (ContainerFurnace.java:51-62)
	// var1 = progressBar (0=cook, 1=burn, 2=currentBurnTime), var2 = value
	void updateProgressBar(int_t progressBar, int_t value);
	
	virtual void tick() override;
	virtual void load(CompoundTag &tag) override;
	virtual void save(CompoundTag &tag) override;
	
	// Beta: stillValid() - checks if player can access (FurnaceTileEntity.java:220-222)
	bool stillValid(Player &player);

private:
	// Beta: canBurn() - checks if can smelt (FurnaceTileEntity.java:167-184)
	bool canBurn();
	
	// Beta: burn() - performs smelting (FurnaceTileEntity.java:186-200)
	void burn();
	
	// Beta: getBurnDuration() - calculates fuel burn time (FurnaceTileEntity.java:202-217)
	int_t getBurnDuration(std::shared_ptr<ItemStack> fuel);
};
