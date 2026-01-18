#include "world/entity/animal/Sheep.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/ClothTile.h"
#include "world/item/ItemStack.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/Mob.h"
#include "nbt/CompoundTag.h"
#include "util/Memory.h"

Sheep::Sheep(Level &level) : Animal(level)
{
	textureName = u"/mob/sheep.png";
	setSize(0.9f, 1.3f);
}

// Beta: Sheep.defineSynchedData() - initializes wool metadata in DataWatcher (Sheep.java:39-43)
// Alpha 1.2.6: EntitySheep.entityInit() - this.dataWatcher.addObject(16, new Byte((byte)0)) (EntitySheep.java:12)
void Sheep::defineSynchedData()
{
	Animal::defineSynchedData();
	// Beta: this.entityData.define(16, new Byte((byte)0)) (Sheep.java:42)
	// Alpha: this.dataWatcher.addObject(16, new Byte((byte)0)) (EntitySheep.java:12)
	dataWatcher.addObject(DATA_WOOL_ID, static_cast<byte_t>(0));
}

bool Sheep::hurt(Entity *source, int_t dmg)
{
	if (!level.isOnline && !isSheared() && source != nullptr && dynamic_cast<Mob*>(source) != nullptr)
	{
		setSheared(true);
		int_t count = 1 + random.nextInt(3);

		for (int_t i = 0; i < count; i++)
		{
			// Alpha: Sheep drops white cloth (no colors in Alpha)
			// Java: new ItemInstance(Tile.cloth.id, 1, ...) - uses tile ID directly (35)
			ItemStack stack(Tile::cloth.id, 1);
			auto itemEntity = std::make_shared<EntityItem>(level, x, y + 1.0f, z, stack);
			itemEntity->yd = itemEntity->yd + random.nextFloat() * 0.05f;
			itemEntity->xd = itemEntity->xd + (random.nextFloat() - random.nextFloat()) * 0.1f;
			itemEntity->zd = itemEntity->zd + (random.nextFloat() - random.nextFloat()) * 0.1f;
			itemEntity->throwTime = 10;
			level.addEntity(itemEntity);
		}
	}

	return Animal::hurt(source, dmg);
}

void Sheep::addAdditionalSaveData(CompoundTag &tag)
{
	Animal::addAdditionalSaveData(tag);
	tag.putBoolean(u"Sheared", isSheared());
}

void Sheep::readAdditionalSaveData(CompoundTag &tag)
{
	Animal::readAdditionalSaveData(tag);
	setSheared(tag.getBoolean(u"Sheared"));
}

jstring Sheep::getAmbientSound()
{
	return u"mob.sheep";
}

jstring Sheep::getHurtSound()
{
	return u"mob.sheep";
}

jstring Sheep::getDeathSound()
{
	return u"mob.sheep";
}

// Beta: Sheep.isSheared() - reads sheared flag from DataWatcher (Sheep.java:100-102)
// Alpha 1.2.6: EntitySheep.getSheared() - returns (this.dataWatcher.getWatchableObjectByte(16) & 16) != 0 (EntitySheep.java:53-55)
bool Sheep::isSheared()
{
	// Beta: return (this.entityData.getByte(16) & 16) != 0; (Sheep.java:101)
	// Alpha: return (this.dataWatcher.getWatchableObjectByte(16) & 16) != 0; (EntitySheep.java:54)
	if (!dataWatcher.hasWatchableObject(DATA_WOOL_ID))
		return false;
	return (dataWatcher.getWatchableObjectByte(DATA_WOOL_ID) & 16) != 0;
}

// Beta: Sheep.setSheared() - writes sheared flag to DataWatcher (Sheep.java:104-110)
// Alpha 1.2.6: EntitySheep.setSheared() - updates DataWatcher (EntitySheep.java:57-65)
void Sheep::setSheared(bool value)
{
	// Beta: byte var2 = this.entityData.getByte(16); (Sheep.java:105)
	//       if (var1) { this.entityData.set(16, (byte)(var2 | 16)); } else { this.entityData.set(16, (byte)(var2 & -17)); } (Sheep.java:106-109)
	// Alpha: byte var2 = this.dataWatcher.getWatchableObjectByte(16); (EntitySheep.java:58)
	//        if(var1) { this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 | 16))); } else { this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 & -17))); } (EntitySheep.java:59-63)
	if (!dataWatcher.hasWatchableObject(DATA_WOOL_ID))
	{
		// If not initialized yet, add it
		dataWatcher.addObject(DATA_WOOL_ID, static_cast<byte_t>(value ? 16 : 0));
		return;
	}
	
	byte_t val = dataWatcher.getWatchableObjectByte(DATA_WOOL_ID);
	if (value)
	{
		dataWatcher.updateObject(DATA_WOOL_ID, static_cast<byte_t>(val | 16));
	}
	else
	{
		// Alpha: ~17 = -17 = 0xFFFFFFEF (clears bit 4)
		dataWatcher.updateObject(DATA_WOOL_ID, static_cast<byte_t>(val & ~16));
	}
}
