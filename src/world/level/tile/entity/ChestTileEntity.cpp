#include "world/level/tile/entity/ChestTileEntity.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "world/level/Level.h"
#include "world/entity/player/Player.h"

// Beta: ChestTileEntity() - items = new ItemInstance[36] (ChestTileEntity.java:10)
ChestTileEntity::ChestTileEntity() : items(36, nullptr)
{
	// Beta: ChestTileEntity has 36 slots but getContainerSize() returns 27 (only first 27 are accessible)
}

// Beta: ChestTileEntity.getItem() - returns items[slot] (ChestTileEntity.java:18-20)
std::shared_ptr<ItemStack> ChestTileEntity::getItem(int_t slot)
{
	if (slot >= 0 && slot < static_cast<int_t>(items.size()))
		return items[slot];
	return nullptr;
}

// Beta: ChestTileEntity.removeItem() - removes items from slot (ChestTileEntity.java:23-42)
std::shared_ptr<ItemStack> ChestTileEntity::removeItem(int_t slot, int_t count)
{
	if (slot >= 0 && slot < static_cast<int_t>(items.size()) && items[slot] != nullptr)
	{
		// Beta: if (this.items[var1].count <= var2) (ChestTileEntity.java:25)
		if (items[slot]->stackSize <= count)
		{
			// Beta: ItemInstance var4 = this.items[var1]; this.items[var1] = null; this.setChanged(); return var4 (ChestTileEntity.java:26-29)
			std::shared_ptr<ItemStack> result = items[slot];
			items[slot] = nullptr;
			setChanged();
			return result;
		}
		else
		{
			// Beta: ItemInstance var3 = this.items[var1].remove(var2) (ChestTileEntity.java:31)
			std::shared_ptr<ItemStack> result = std::make_shared<ItemStack>(items[slot]->remove(count));
			// Beta: if (this.items[var1].count == 0) this.items[var1] = null (ChestTileEntity.java:32-34)
			if (items[slot]->stackSize == 0)
			{
				items[slot] = nullptr;
			}
			// Beta: this.setChanged(); return var3 (ChestTileEntity.java:36-37)
			setChanged();
			return result;
		}
	}
	return nullptr;
}

// Beta: ChestTileEntity.setItem() - sets item in slot (ChestTileEntity.java:45-52)
void ChestTileEntity::setItem(int_t slot, std::shared_ptr<ItemStack> itemStack)
{
	if (slot >= 0 && slot < static_cast<int_t>(items.size()))
	{
		// Beta: this.items[var1] = var2 (ChestTileEntity.java:46)
		items[slot] = itemStack;
		// Beta: if (var2 != null && var2.count > this.getMaxStackSize()) var2.count = this.getMaxStackSize() (ChestTileEntity.java:47-49)
		if (itemStack != nullptr && itemStack->stackSize > getMaxStackSize())
		{
			itemStack->stackSize = getMaxStackSize();
		}
		// Beta: this.setChanged() (ChestTileEntity.java:51)
		setChanged();
	}
}

// Beta: ChestTileEntity.setItemDirect() - direct setter for world generation (doesn't call setChanged)
void ChestTileEntity::setItemDirect(int_t slot, std::shared_ptr<ItemStack> itemStack)
{
	// Direct setter for world generation (doesn't call setChanged to avoid bad_weak_ptr)
	if (slot >= 0 && slot < static_cast<int_t>(items.size()))
	{
		items[slot] = itemStack;
		if (itemStack != nullptr && itemStack->stackSize > getMaxStackSize())
			itemStack->stackSize = getMaxStackSize();
		// Don't call setChanged() - level is not set during world generation
	}
}

// Beta: ChestTileEntity.load() - loads from NBT (ChestTileEntity.java:60-72)
void ChestTileEntity::load(CompoundTag &tag)
{
	TileEntity::load(tag);
	// Beta: ListTag var2 = var1.getList("Items") (ChestTileEntity.java:62)
	std::shared_ptr<ListTag> itemsList = tag.getList(u"Items");
	// Beta: this.items = new ItemInstance[this.getContainerSize()] (ChestTileEntity.java:63)
	items.assign(36, nullptr);
	
	// Beta: for (int var3 = 0; var3 < var2.size(); var3++) (ChestTileEntity.java:65)
	for (int_t i = 0; i < itemsList->size(); i++)
	{
		// Beta: CompoundTag var4 = (CompoundTag)var2.get(var3) (ChestTileEntity.java:66)
		std::shared_ptr<CompoundTag> itemTag = std::static_pointer_cast<CompoundTag>(itemsList->get(i));
		// Beta: int var5 = var4.getByte("Slot") & 255 (ChestTileEntity.java:67)
		int_t slot = itemTag->getByte(u"Slot") & 255;
		if (slot >= 0 && slot < static_cast<int_t>(items.size()))
		{
			// Beta: this.items[var5] = new ItemInstance(var4) (ChestTileEntity.java:69)
			items[slot] = std::make_shared<ItemStack>();
			items[slot]->load(*itemTag);
		}
	}
}

// Beta: ChestTileEntity.save() - saves to NBT (ChestTileEntity.java:75-89)
void ChestTileEntity::save(CompoundTag &tag)
{
	TileEntity::save(tag);
	// Beta: ListTag var2 = new ListTag() (ChestTileEntity.java:77)
	std::shared_ptr<ListTag> itemsList = std::make_shared<ListTag>();
	
	// Beta: for (int var3 = 0; var3 < this.items.length; var3++) (ChestTileEntity.java:79)
	for (int_t i = 0; i < static_cast<int_t>(items.size()); i++)
	{
		if (items[i] != nullptr)
		{
			// Beta: CompoundTag var4 = new CompoundTag() (ChestTileEntity.java:81)
			std::shared_ptr<CompoundTag> itemTag = std::make_shared<CompoundTag>();
			// Beta: var4.putByte("Slot", (byte)var3) (ChestTileEntity.java:82)
			itemTag->putByte(u"Slot", static_cast<byte_t>(i));
			// Beta: this.items[var3].save(var4) (ChestTileEntity.java:83)
			items[i]->save(*itemTag);
			// Beta: var2.add(var4) (ChestTileEntity.java:84)
			itemsList->add(itemTag);
		}
	}
	
	// Beta: var1.put("Items", var2) (ChestTileEntity.java:88)
	tag.put(u"Items", itemsList);
}

// Beta: ChestTileEntity.stillValid() - checks if player can access (ChestTileEntity.java:100-102)
bool ChestTileEntity::stillValid(Player &player)
{
	if (level == nullptr)
		return false;
	
	// Beta: return this.level.getTileEntity(this.x, this.y, this.z) != this ? false : ... (ChestTileEntity.java:101)
	std::shared_ptr<TileEntity> te = level->getTileEntity(x, y, z);
	if (te.get() != this)
		return false;
	
	// Beta: !(var1.distanceToSqr(this.x + 0.5, this.y + 0.5, this.z + 0.5) > 64.0) (ChestTileEntity.java:101)
	double distSqr = player.distanceToSqr(x + 0.5, y + 0.5, z + 0.5);
	return !(distSqr > 64.0);
}
