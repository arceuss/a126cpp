#include "world/item/ItemStack.h"
#include "world/item/Item.h"
#include "world/level/tile/Tile.h"
#include "world/entity/Mob.h"
#include "world/entity/Entity.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "Facing.h"
#include "nbt/CompoundTag.h"

ItemStack::ItemStack()
{
}

ItemStack::ItemStack(int_t shiftedIndex) : ItemStack(shiftedIndex, 1)
{
}

ItemStack::ItemStack(int_t shiftedIndex, int_t count)
{
	this->stackSize = 0;
	this->itemID = shiftedIndex;
	this->stackSize = count;
	this->itemDamage = 0;
}

ItemStack::ItemStack(int_t shiftedIndex, int_t count, int_t damage)
{
	this->stackSize = 0;
	this->itemID = shiftedIndex;
	this->stackSize = count;
	this->itemDamage = damage;
}

// Alpha: ItemStack(NBTTagCompound var1) - construct from NBT (ItemStack.java:46-49)
ItemStack::ItemStack(CompoundTag &tag)
{
	this->stackSize = 0;
	this->load(tag);
}

Item *ItemStack::getItem() const
{
	if (itemID < 0 || itemID >= 32000)
		return nullptr;
	return Item::itemsList[itemID];
}

float ItemStack::getStrVsBlock(Tile &tile)
{
	Item *item = getItem();
	if (item == nullptr)
		return 1.0F;
	return item->getStrVsBlock(*this, tile);
}

bool ItemStack::canHarvestBlock(Tile &tile)
{
	Item *item = getItem();
	if (item == nullptr)
		return false;
	return item->canHarvestBlock(tile);
}

void ItemStack::damageItem(int_t amount)
{
	// Alpha: damageItem() increments itemDamage, breaks item when exceeded (ItemStack.java:97-108)
	if (!isItemStackDamageable())
		return;
	
	itemDamage += amount;
	if (itemDamage > getMaxDamage())
	{
		--stackSize;
		if (stackSize < 0)
			stackSize = 0;
		itemDamage = 0;
	}
}

void ItemStack::hitEntity(Mob &entity)
{
	// Alpha: hitEntity calls item.hitEntity, which damages item by 2 (ItemTool.java:33-35)
	Item *item = getItem();
	if (item != nullptr)
		item->hitEntity(*this, entity);
}

void ItemStack::hitBlock(int_t blockID, int_t x, int_t y, int_t z)
{
	// Alpha: hitBlock calls item.hitBlock (ItemStack.java:114-116, PlayerControllerSP.java:29)
	// Signature: hitBlock(blockID, x, y, z) -> Item.hitBlock(stack, blockID, x, y, z)
	Item *item = getItem();
	if (item != nullptr)
		item->hitBlock(*this, blockID, x, y, z);
}

int_t ItemStack::getMaxStackSize() const
{
	Item *item = getItem();
	if (item == nullptr)
		return 64;
	return item->getMaxStackSize();
}

int_t ItemStack::getMaxDamage() const
{
	Item *item = getItem();
	if (item == nullptr)
		return 0;
	return item->getMaxDamage();
}

bool ItemStack::isItemStackDamageable() const
{
	return getMaxDamage() > 0;
}

bool ItemStack::isItemDamaged() const
{
	return isItemStackDamageable() && itemDamage > 0;
}

bool ItemStack::isStackable() const
{
	// Beta: ItemInstance.isStackable() - returns true if item can stack (ItemInstance.java:93-95)
	// Beta: getMaxStackSize() > 1 && (!isDamageableItem() || !isDamaged())
	return getMaxStackSize() > 1 && (!isItemStackDamageable() || !isItemDamaged());
}

int_t ItemStack::getIcon() const
{
	// Beta: ItemStack.getIconIndex() - calls getItem().getIconIndex(this)
	// For blocks (itemID < 256), if there's no Item entry, return the tile's texture index
	if (itemID < 256)
	{
		Tile *tile = Tile::tiles[itemID];
		if (tile != nullptr)
		{
			// Return the tile's texture index (typically from Facing::NORTH for item icon)
			return tile->getTexture(Facing::NORTH, itemDamage);
		}
	}
	
	Item *item = getItem();
	if (item == nullptr)
		return 0;
	return item->getIconIndex(*this);
}

// Beta: ItemStack.useOn() - uses item on block (ItemInstance.java:64-66)
bool ItemStack::useOn(Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Delegates to item.useOn() (ItemInstance.java:65)
	Item *item = getItem();
	
	// Beta: Handle block placement directly for blocks (itemID < 256) when there's no Item instance
	// This matches TileItem::useOn behavior (TileItem.java:20-72)
	if (item == nullptr && itemID < 256)
	{
		// Beta: Adjust position based on face (TileItem.java:25-47)
		if (face == Facing::DOWN)
			y--;
		else if (face == Facing::UP)
			y++;
		else if (face == Facing::NORTH)
			z--;
		else if (face == Facing::SOUTH)
			z++;
		else if (face == Facing::WEST)
			x--;
		else if (face == Facing::EAST)
			x++;
		
		// Beta: Check if stack is empty (TileItem.java:50-51)
		if (stackSize == 0)
			return false;
		
		Tile *tile = Tile::tiles[itemID];
		if (tile == nullptr)
			return false;
		
		// Beta: Check if we can place the block (TileItem.java:53)
		// Beta: level.mayPlace(this.tileId, var4, var5, var6, false) (TileItem.java:53)
		if (level.mayPlace(itemID, x, y, z, false))
		{
		// Beta: Place block with data (TileItem.java:55)
		// Beta: this.getLevelDataForAuxValue(var1.getAuxValue()) (TileItem.java:55)
		// Since blocks without Item instances don't have getLevelDataForAuxValue, use 0 (default Item.getLevelDataForAuxValue returns 0)
		int_t data = 0;  // Beta: Item.getLevelDataForAuxValue returns 0 by default (Item.java:178-180)
		if (level.setTileAndData(x, y, z, itemID, data))
		{
			// Beta: Call tile placement callbacks (TileItem.java:56-57)
			tile->setPlacedOnFace(level, x, y, z, face);
			tile->setPlacedBy(level, x, y, z, player);  // Beta: Tile.setPlacedBy() called after placement (TileItem.java:57)
			
			// Alpha 1.2.6: Play placement sound only if NOT multiplayer (ItemBlock.java:49-51)
			// Java: if(!var3.multiplayerWorld) { var3.playSoundEffect(...); }
			if (!level.isOnline)
			{
				const Tile::SoundType *soundType = tile->getSoundType();
				if (soundType != nullptr)
				{
					jstring stepSound = soundType->getStepSound();
					float volume = (soundType->getVolume() + 1.0f) / 2.0f;
					float pitch = soundType->getPitch() * 0.8f;
					level.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, stepSound, volume, pitch);
				}
			}
				
				// Beta: Decrease stack size (TileItem.java:66)
				stackSize--;
			}
		}
		
		return true;
	}
	
	if (item == nullptr)
		return false;
	return item->useOn(*this, player, level, x, y, z, face);
}

// Beta: ItemStack.use() - uses item (ItemInstance.java:72-74)
ItemStack ItemStack::use(Level &level, Player &player)
{
	// Beta: Delegates to item.use() (ItemInstance.java:73)
	Item *item = getItem();
	if (item == nullptr)
		return *this;
	return item->use(*this, level, player);
}

// Beta: ItemStack.save() - saves to NBT (ItemInstance.java:76-81)
void ItemStack::save(CompoundTag &tag)
{
	tag.putShort(u"id", (short_t)itemID);
	tag.putByte(u"Count", (byte_t)stackSize);
	tag.putShort(u"Damage", (short_t)itemDamage);
}

// Beta: ItemStack.load() - loads from NBT (ItemInstance.java:83-87)
void ItemStack::load(CompoundTag &tag)
{
	itemID = tag.getShort(u"id");
	stackSize = tag.getByte(u"Count");
	itemDamage = tag.getShort(u"Damage");
}

// Beta: ItemStack.remove() - removes count items and returns new stack (ItemInstance.java:51-54)
ItemStack ItemStack::remove(int_t count)
{
	this->stackSize -= count;
	return ItemStack(this->itemID, count, this->itemDamage);
}

// Beta: ItemStack.sameItem() - checks if same item ID and damage (ItemInstance.java:178-180)
bool ItemStack::sameItem(const ItemStack &other) const
{
	return this->itemID == other.itemID && this->itemDamage == other.itemDamage;
}

// Beta: ItemStack.getAttackDamage(Entity var1) - gets attack damage from item (ItemInstance.java:143-145)
int_t ItemStack::getAttackDamage(Entity &entity)
{
	Item *item = getItem();
	if (item == nullptr)
		return 1;  // Beta: default attack damage = 1 (Item.java:207)
	return item->getAttackDamage(entity);  // Beta: return Item.items[this.id].getAttackDamage(var1) (ItemInstance.java:144)
}
