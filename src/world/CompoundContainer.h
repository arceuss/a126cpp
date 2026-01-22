#pragma once

#include "world/item/ItemStack.h"
#include "java/String.h"
#include "java/Type.h"
#include <memory>

class Player;

// Beta 1.2 CompoundContainer - combines two containers into one (for double chests)
// Reference: beta1.2/minecraft/src/net/minecraft/world/CompoundContainer.java
class CompoundContainer
{
private:
	jstring name;
	std::shared_ptr<class ChestTileEntity> c1;  // First chest
	std::shared_ptr<class ChestTileEntity> c2;  // Second chest (can be null for single chest)

public:
	// Beta: CompoundContainer(String var1, Container var2, Container var3) (CompoundContainer.java:11-15)
	CompoundContainer(jstring name, std::shared_ptr<class ChestTileEntity> c1, std::shared_ptr<class ChestTileEntity> c2);
	
	// Beta: getContainerSize() - returns sum of both containers (CompoundContainer.java:18-20)
	int_t getContainerSize() const;
	
	// Beta: getName() - returns name (CompoundContainer.java:23-25)
	jstring getName() const { return name; }
	
	// Beta: getItem(int var1) - gets item from appropriate container (CompoundContainer.java:28-30)
	std::shared_ptr<ItemStack> getItem(int_t slot);
	
	// Beta: removeItem(int var1, int var2) - removes item from appropriate container (CompoundContainer.java:33-35)
	std::shared_ptr<ItemStack> removeItem(int_t slot, int_t count);
	
	// Beta: setItem(int var1, ItemInstance var2) - sets item in appropriate container (CompoundContainer.java:38-44)
	void setItem(int_t slot, std::shared_ptr<ItemStack> itemStack);
	
	// Beta: getMaxStackSize() - returns max stack size (CompoundContainer.java:47-49)
	int_t getMaxStackSize() const { return 64; }
	
	// Beta: setChanged() - calls setChanged on both containers (CompoundContainer.java:52-55)
	void setChanged();
	
	// Beta: stillValid(Player var1) - checks if both containers are valid (CompoundContainer.java:58-60)
	bool stillValid(Player &player);
};
