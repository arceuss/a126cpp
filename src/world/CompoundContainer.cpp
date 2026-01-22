#include "world/CompoundContainer.h"

#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/entity/player/Player.h"

// Beta: CompoundContainer(String var1, Container var2, Container var3) (CompoundContainer.java:11-15)
CompoundContainer::CompoundContainer(jstring name, std::shared_ptr<ChestTileEntity> c1, std::shared_ptr<ChestTileEntity> c2)
	: name(name), c1(c1), c2(c2)
{
}

// Beta: getContainerSize() - returns sum of both containers (CompoundContainer.java:18-20)
int_t CompoundContainer::getContainerSize() const
{
	int_t size1 = c1 ? c1->getContainerSize() : 0;
	int_t size2 = c2 ? c2->getContainerSize() : 0;
	return size1 + size2;
}

// Beta: getItem(int var1) - gets item from appropriate container (CompoundContainer.java:28-30)
std::shared_ptr<ItemStack> CompoundContainer::getItem(int_t slot)
{
	int_t size1 = c1 ? c1->getContainerSize() : 0;
	if (slot >= size1)
	{
		// Beta: return this.c2.getItem(var1 - this.c1.getContainerSize()) (CompoundContainer.java:29)
		return c2 ? c2->getItem(slot - size1) : nullptr;
	}
	else
	{
		// Beta: return this.c1.getItem(var1) (CompoundContainer.java:29)
		return c1 ? c1->getItem(slot) : nullptr;
	}
}

// Beta: removeItem(int var1, int var2) - removes item from appropriate container (CompoundContainer.java:33-35)
std::shared_ptr<ItemStack> CompoundContainer::removeItem(int_t slot, int_t count)
{
	int_t size1 = c1 ? c1->getContainerSize() : 0;
	if (slot >= size1)
	{
		// Beta: return this.c2.removeItem(var1 - this.c1.getContainerSize(), var2) (CompoundContainer.java:34)
		return c2 ? c2->removeItem(slot - size1, count) : nullptr;
	}
	else
	{
		// Beta: return this.c1.removeItem(var1, var2) (CompoundContainer.java:34)
		return c1 ? c1->removeItem(slot, count) : nullptr;
	}
}

// Beta: setItem(int var1, ItemInstance var2) - sets item in appropriate container (CompoundContainer.java:38-44)
void CompoundContainer::setItem(int_t slot, std::shared_ptr<ItemStack> itemStack)
{
	int_t size1 = c1 ? c1->getContainerSize() : 0;
	if (slot >= size1)
	{
		// Beta: this.c2.setItem(var1 - this.c1.getContainerSize(), var2) (CompoundContainer.java:40)
		if (c2)
			c2->setItem(slot - size1, itemStack);
	}
	else
	{
		// Beta: this.c1.setItem(var1, var2) (CompoundContainer.java:42)
		if (c1)
			c1->setItem(slot, itemStack);
	}
}

// Beta: setChanged() - calls setChanged on both containers (CompoundContainer.java:52-55)
void CompoundContainer::setChanged()
{
	if (c1)
		c1->setChanged();
	if (c2)
		c2->setChanged();
}

// Beta: stillValid(Player var1) - checks if both containers are valid (CompoundContainer.java:58-60)
bool CompoundContainer::stillValid(Player &player)
{
	bool valid1 = c1 ? c1->stillValid(player) : true;
	bool valid2 = c2 ? c2->stillValid(player) : true;
	// Beta: return this.c1.stillValid(var1) && this.c2.stillValid(var1) (CompoundContainer.java:59)
	return valid1 && valid2;
}
