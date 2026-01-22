#include "world/level/tile/entity/FurnaceTileEntity.h"

#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "world/level/Level.h"
#include "world/level/tile/FurnaceTile.h"
#include "world/level/tile/Tile.h"
#include "world/item/Item.h"
#include "world/item/Items.h"
#include "world/item/crafting/FurnaceRecipes.h"
#include "world/entity/player/Player.h"
#include "world/level/material/Material.h"

// Beta: FurnaceTileEntity() - constructor (FurnaceTileEntity.java:14-20)
FurnaceTileEntity::FurnaceTileEntity() : items(3, nullptr)
{
	litTime = 0;
	litDuration = 0;
	tickCount = 0;
}

// Beta: FurnaceTileEntity.getItem() - returns items[slot] (FurnaceTileEntity.java:27-29)
std::shared_ptr<ItemStack> FurnaceTileEntity::getItem(int_t slot)
{
	if (slot >= 0 && slot < static_cast<int_t>(items.size()))
		return items[slot];
	return nullptr;
}

// Beta: FurnaceTileEntity.removeItem() - removes items from slot (FurnaceTileEntity.java:32-49)
std::shared_ptr<ItemStack> FurnaceTileEntity::removeItem(int_t slot, int_t count)
{
	if (slot >= 0 && slot < static_cast<int_t>(items.size()) && items[slot] != nullptr)
	{
		if (items[slot]->stackSize <= count)
		{
			std::shared_ptr<ItemStack> result = items[slot];
			items[slot] = nullptr;
			return result;
		}
		else
		{
			std::shared_ptr<ItemStack> result = std::make_shared<ItemStack>(items[slot]->remove(count));
			if (items[slot]->stackSize == 0)
			{
				items[slot] = nullptr;
			}
			return result;
		}
	}
	return nullptr;
}

// Beta: FurnaceTileEntity.setItem() - sets item in slot (FurnaceTileEntity.java:52-57)
void FurnaceTileEntity::setItem(int_t slot, std::shared_ptr<ItemStack> itemStack)
{
	if (slot >= 0 && slot < static_cast<int_t>(items.size()))
	{
		items[slot] = itemStack;
		if (itemStack != nullptr && itemStack->stackSize > getMaxStackSize())
		{
			itemStack->stackSize = getMaxStackSize();
		}
	}
}

// Beta: FurnaceTileEntity.load() - loads from NBT (FurnaceTileEntity.java:65-81)
void FurnaceTileEntity::load(CompoundTag &tag)
{
	TileEntity::load(tag);
	
	// Beta: ListTag var2 = var1.getList("Items") (FurnaceTileEntity.java:67)
	std::shared_ptr<ListTag> itemsList = tag.getList(u"Items");
	items.assign(3, nullptr);
	
	// Beta: for (int var3 = 0; var3 < var2.size(); var3++) (FurnaceTileEntity.java:70)
	for (int_t i = 0; i < itemsList->size(); i++)
	{
		// Beta: CompoundTag var4 = (CompoundTag)var2.get(var3) (FurnaceTileEntity.java:71)
		std::shared_ptr<CompoundTag> itemTag = std::static_pointer_cast<CompoundTag>(itemsList->get(i));
		// Beta: byte var5 = var4.getByte("Slot") (FurnaceTileEntity.java:72)
		byte_t slot = itemTag->getByte(u"Slot");
		if (slot >= 0 && slot < static_cast<byte_t>(items.size()))
		{
			// Beta: this.items[var5] = new ItemInstance(var4) (FurnaceTileEntity.java:74)
			items[slot] = std::make_shared<ItemStack>();
			items[slot]->load(*itemTag);
		}
	}
	
	// Beta: this.litTime = var1.getShort("BurnTime") (FurnaceTileEntity.java:78)
	litTime = tag.getShort(u"BurnTime");
	// Beta: this.tickCount = var1.getShort("CookTime") (FurnaceTileEntity.java:79)
	tickCount = tag.getShort(u"CookTime");
	// Beta: this.litDuration = this.getBurnDuration(this.items[1]) (FurnaceTileEntity.java:80)
	litDuration = getBurnDuration(items[1]);
}

// Beta: FurnaceTileEntity.save() - saves to NBT (FurnaceTileEntity.java:84-100)
void FurnaceTileEntity::save(CompoundTag &tag)
{
	TileEntity::save(tag);
	// Beta: var1.putShort("BurnTime", (short)this.litTime) (FurnaceTileEntity.java:86)
	tag.putShort(u"BurnTime", static_cast<short_t>(litTime));
	// Beta: var1.putShort("CookTime", (short)this.tickCount) (FurnaceTileEntity.java:87)
	tag.putShort(u"CookTime", static_cast<short_t>(tickCount));
	
	// Beta: ListTag var2 = new ListTag() (FurnaceTileEntity.java:88)
	std::shared_ptr<ListTag> itemsList = std::make_shared<ListTag>();
	
	// Beta: for (int var3 = 0; var3 < this.items.length; var3++) (FurnaceTileEntity.java:90)
	for (int_t i = 0; i < static_cast<int_t>(items.size()); i++)
	{
		if (items[i] != nullptr)
		{
			// Beta: CompoundTag var4 = new CompoundTag() (FurnaceTileEntity.java:92)
			std::shared_ptr<CompoundTag> itemTag = std::make_shared<CompoundTag>();
			// Beta: var4.putByte("Slot", (byte)var3) (FurnaceTileEntity.java:93)
			itemTag->putByte(u"Slot", static_cast<byte_t>(i));
			// Beta: this.items[var3].save(var4) (FurnaceTileEntity.java:94)
			items[i]->save(*itemTag);
			// Beta: var2.add(var4) (FurnaceTileEntity.java:95)
			itemsList->add(itemTag);
		}
	}
	
	// Beta: var1.put("Items", var2) (FurnaceTileEntity.java:99)
	tag.put(u"Items", itemsList);
}

// Beta: FurnaceTileEntity.getBurnProgress() - returns cooking progress (FurnaceTileEntity.java:107-109)
int_t FurnaceTileEntity::getBurnProgress(int_t scale) const
{
	return tickCount * scale / BURN_INTERVAL;
}

// Beta: FurnaceTileEntity.getLitProgress() - returns fuel burn progress (FurnaceTileEntity.java:111-117)
int_t FurnaceTileEntity::getLitProgress(int_t scale)
{
	if (litDuration == 0)
	{
		litDuration = 200;
	}
	return litTime * scale / litDuration;
}

// Alpha 1.2.6: Update progress bar values from Packet105UpdateProgressbar
// Java: ContainerFurnace.func_20112_a(int var1, int var2) (ContainerFurnace.java:51-62)
// var1 = progressBar (0=cookTime, 1=burnTime, 2=currentItemBurnTime), var2 = value
// Maps to FurnaceTileEntity: 0=tickCount (furnaceCookTime), 1=litTime (furnaceBurnTime), 2=litDuration (currentItemBurnTime)
void FurnaceTileEntity::updateProgressBar(int_t progressBar, int_t value)
{
	// Java: ContainerFurnace.func_20112_a() line 52-62
	// if(var1 == 0) { this.furnace.furnaceCookTime = var2; }  (line 52-54)
	// if(var1 == 1) { this.furnace.furnaceBurnTime = var2; }  (line 56-58)
	// if(var1 == 2) { this.furnace.currentItemBurnTime = var2; }  (line 60-62)
	if (progressBar == 0)
	{
		// Progress bar 0: Cook time (furnaceCookTime) → tickCount
		tickCount = value;
	}
	else if (progressBar == 1)
	{
		// Progress bar 1: Burn time remaining (furnaceBurnTime) → litTime
		litTime = value;
	}
	else if (progressBar == 2)
	{
		// Progress bar 2: Current item burn time (currentItemBurnTime) → litDuration
		litDuration = value;
	}
}

// Beta: FurnaceTileEntity.tick() - smelting logic (FurnaceTileEntity.java:124-165)
void FurnaceTileEntity::tick()
{
	bool wasLit = litTime > 0;
	bool changed = false;
	
	// Beta: if (this.litTime > 0) this.litTime-- (FurnaceTileEntity.java:127-129)
	if (litTime > 0)
	{
		litTime--;
	}
	
	// Beta: if (!this.level.isOnline) (FurnaceTileEntity.java:131)
	if (level != nullptr && !level->isOnline)
	{
		// Beta: if (this.litTime == 0 && this.canBurn()) (FurnaceTileEntity.java:132)
		if (litTime == 0 && canBurn())
		{
			// Beta: this.litDuration = this.litTime = this.getBurnDuration(this.items[1]) (FurnaceTileEntity.java:133)
			litDuration = litTime = getBurnDuration(items[1]);
			if (litTime > 0)
			{
				changed = true;
				// Beta: if (this.items[1] != null) (FurnaceTileEntity.java:136)
				if (items[1] != nullptr)
				{
					// Beta: this.items[1].count-- (FurnaceTileEntity.java:137)
					items[1]->stackSize--;
					// Beta: if (this.items[1].count == 0) this.items[1] = null (FurnaceTileEntity.java:138-140)
					if (items[1]->stackSize == 0)
					{
						items[1] = nullptr;
					}
				}
			}
		}
		
		// Beta: if (this.isLit() && this.canBurn()) (FurnaceTileEntity.java:145)
		if (isLit() && canBurn())
		{
			// Beta: this.tickCount++ (FurnaceTileEntity.java:146)
			tickCount++;
			// Beta: if (this.tickCount == 200) (FurnaceTileEntity.java:147)
			if (tickCount == BURN_INTERVAL)
			{
				// Beta: this.tickCount = 0; this.burn(); var2 = true (FurnaceTileEntity.java:148-150)
				tickCount = 0;
				burn();
				changed = true;
			}
		}
		else
		{
			// Beta: this.tickCount = 0 (FurnaceTileEntity.java:153)
			tickCount = 0;
		}
		
		// Beta: if (var1 != this.litTime > 0) (FurnaceTileEntity.java:156)
		if (wasLit != (litTime > 0))
		{
			changed = true;
			// Beta: FurnaceTile.setLit(this.litTime > 0, this.level, this.x, this.y, this.z) (FurnaceTileEntity.java:158)
			FurnaceTile::setLit(litTime > 0, *level, x, y, z);
		}
	}
	
	// Beta: if (var2) this.setChanged() (FurnaceTileEntity.java:162-164)
	if (changed)
	{
		setChanged();
	}
}

// Beta: FurnaceTileEntity.canBurn() - checks if can smelt (FurnaceTileEntity.java:167-184)
bool FurnaceTileEntity::canBurn()
{
	// Beta: if (this.items[0] == null) return false (FurnaceTileEntity.java:168-170)
	if (items[0] == nullptr)
		return false;
	
	// Beta: ItemInstance var1 = FurnaceRecipes.getInstance().getResult(this.items[0].getItem().id) (FurnaceTileEntity.java:171)
	// In Java, items[0].getItem().id returns the item ID (shifted index)
	// In C++, we use itemID directly since it's already the shifted index
	std::shared_ptr<ItemStack> result = FurnaceRecipes::getInstance().getResult(items[0]->itemID);
	if (result == nullptr || result->isEmpty())
		return false;
	
	// Beta: else if (this.items[2] == null) return true (FurnaceTileEntity.java:174-175)
	if (items[2] == nullptr)
		return true;
	
	// Beta: else if (!this.items[2].sameItem(var1)) return false (FurnaceTileEntity.java:176-178)
	if (!items[2]->sameItem(*result))
		return false;
	
	// Beta: return this.items[2].count < this.getMaxStackSize() && ... (FurnaceTileEntity.java:179-181)
	return items[2]->stackSize < getMaxStackSize() && items[2]->stackSize < items[2]->getMaxStackSize()
		? true
		: items[2]->stackSize < result->getMaxStackSize();
}

// Beta: FurnaceTileEntity.burn() - performs smelting (FurnaceTileEntity.java:186-200)
void FurnaceTileEntity::burn()
{
	if (canBurn())
	{
		// Beta: ItemInstance var1 = FurnaceRecipes.getInstance().getResult(this.items[0].getItem().id) (FurnaceTileEntity.java:188)
		// In Java, items[0].getItem().id returns the item ID (shifted index)
		// In C++, we use itemID directly since it's already the shifted index
		std::shared_ptr<ItemStack> result = FurnaceRecipes::getInstance().getResult(items[0]->itemID);
		if (result == nullptr || result->isEmpty())
			return;
		
		// Beta: if (this.items[2] == null) this.items[2] = var1.copy() (FurnaceTileEntity.java:189-191)
		if (items[2] == nullptr)
		{
			items[2] = std::make_shared<ItemStack>(result->copy());
		}
		// Beta: else if (this.items[2].id == var1.id) this.items[2].count++ (FurnaceTileEntity.java:192-193)
		// Note: Beta only compares id, not auxValue (damage)
		else if (items[2]->itemID == result->itemID)
		{
			items[2]->stackSize++;
		}
		
		// Beta: this.items[0].count-- (FurnaceTileEntity.java:195)
		if (items[0] != nullptr)
		{
			items[0]->stackSize--;
			// Beta: if (this.items[0].count <= 0) this.items[0] = null (FurnaceTileEntity.java:196-198)
			if (items[0]->stackSize <= 0)
			{
				items[0] = nullptr;
			}
		}
	}
}

// Beta: FurnaceTileEntity.getBurnDuration() - calculates fuel burn time (FurnaceTileEntity.java:202-217)
int_t FurnaceTileEntity::getBurnDuration(std::shared_ptr<ItemStack> fuel)
{
	if (fuel == nullptr)
		return 0;
	
	// Beta: int var2 = var1.getItem().id (FurnaceTileEntity.java:206)
	int_t itemId = fuel->itemID;
	
	// Beta: if (var2 < 256 && Tile.tiles[var2].material == Material.wood) return 300 (FurnaceTileEntity.java:207-209)
	if (itemId < 256 && Tile::tiles[itemId] != nullptr && Tile::tiles[itemId]->material == Material::wood)
		return 300;
	
	// Beta: else if (var2 == Item.stick.id) return 100 (FurnaceTileEntity.java:210)
	Item *item = fuel->getItem();
	if (item != nullptr)
	{
		// Beta: Check Item.stick.id (FurnaceTileEntity.java:210)
		if (item == Items::stick)
			return 100;
		
		// Beta: else if (var2 == Item.coal.id) return 1600 (FurnaceTileEntity.java:212)
		if (item == Items::coal)
			return 1600;
		
		// Beta: else return var2 == Item.bucket_lava.id ? 20000 : 0 (FurnaceTileEntity.java:214)
		if (item == Items::bucketLava)
			return 20000;
	}
	
	return 0;
}

// Beta: FurnaceTileEntity.stillValid() - checks if player can access (FurnaceTileEntity.java:220-222)
bool FurnaceTileEntity::stillValid(Player &player)
{
	if (level == nullptr)
		return false;
	
	// Beta: return this.level.getTileEntity(this.x, this.y, this.z) != this ? false : ... (FurnaceTileEntity.java:221)
	std::shared_ptr<TileEntity> te = level->getTileEntity(x, y, z);
	if (te.get() != this)
		return false;
	
	// Beta: !(var1.distanceToSqr(this.x + 0.5, this.y + 0.5, this.z + 0.5) > 64.0) (FurnaceTileEntity.java:221)
	double distSqr = player.distanceToSqr(x + 0.5, y + 0.5, z + 0.5);
	return !(distSqr > 64.0);
}
