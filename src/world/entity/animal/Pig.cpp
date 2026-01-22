#include "world/entity/animal/Pig.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "world/entity/player/Player.h"
#include "nbt/CompoundTag.h"

Pig::Pig(Level &level) : Animal(level)
{
	textureName = u"/mob/pig.png";
	setSize(0.9f, 0.9f);
}

void Pig::defineSynchedData()
{
	Animal::defineSynchedData();
	entityDataSaddle = 0;  // TODO: Replace with proper SynchedEntityData
}

void Pig::addAdditionalSaveData(CompoundTag &tag)
{
	Animal::addAdditionalSaveData(tag);
	tag.putBoolean(u"Saddle", hasSaddle());
}

void Pig::readAdditionalSaveData(CompoundTag &tag)
{
	Animal::readAdditionalSaveData(tag);
	setSaddle(tag.getBoolean(u"Saddle"));
}

jstring Pig::getAmbientSound()
{
	return u"mob.pig";
}

jstring Pig::getHurtSound()
{
	return u"mob.pig";
}

jstring Pig::getDeathSound()
{
	return u"mob.pigdeath";
}

int_t Pig::getDeathLoot()
{
	if (Items::porkRaw != nullptr)
		return Items::porkRaw->getShiftedIndex();
	return 0;
}

bool Pig::interact(Player &player)
{
	if (!hasSaddle() || level.isOnline || (rider != nullptr && rider.get() != &player))
	{
		return false;
	}
	else
	{
		// TODO: Implement Entity.ride() - player.ride(this)
		// For now, just return true
		return true;
	}
}

bool Pig::hasSaddle()
{
	return (entityDataSaddle & 1) != 0;
}

void Pig::setSaddle(bool value)
{
	if (value)
	{
		entityDataSaddle = 1;
	}
	else
	{
		entityDataSaddle = 0;
	}
}
