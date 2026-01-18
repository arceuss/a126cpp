#include "world/entity/monster/Creeper.h"

#include "world/level/Level.h"
#include "world/entity/monster/Skeleton.h"
#include "world/item/Items.h"
#include "nbt/CompoundTag.h"

Creeper::Creeper(Level &level) : Monster(level)
{
	textureName = u"/mob/creeper.png";
}

void Creeper::defineSynchedData()
{
	Monster::defineSynchedData();
	// TODO: entityData.define(16, (byte)-1); when SynchedEntityData is implemented
	swellDir = -1;
}

void Creeper::addAdditionalSaveData(CompoundTag &tag)
{
	Monster::addAdditionalSaveData(tag);
}

void Creeper::readAdditionalSaveData(CompoundTag &tag)
{
	Monster::readAdditionalSaveData(tag);
}

void Creeper::tick()
{
	oldSwell = swell;
	if (level.isOnline)
	{
		int_t var1 = getSwellDir();
		if (var1 > 0 && swell == 0)
		{
			level.playSound(this, u"random.fuse", 1.0f, 0.5f);
		}

		swell += var1;
		if (swell < 0)
		{
			swell = 0;
		}

		if (swell >= MAX_SWELL)
		{
			swell = MAX_SWELL;
		}
	}

	Monster::tick();
}

jstring Creeper::getHurtSound()
{
	return u"mob.creeper";
}

jstring Creeper::getDeathSound()
{
	return u"mob.creeperdeath";
}

void Creeper::die(Entity *source)
{
	Monster::die(source);
	if (source != nullptr && dynamic_cast<Skeleton*>(source) != nullptr)
	{
		// newb12: Creeper.die() - drops record if killed by skeleton (Creeper.java:68-72)
		int_t recordId = Items::record01 != nullptr ? Items::record01->getShiftedIndex() : 0;
		if (recordId != 0)
		{
			spawnAtLocation(recordId + random.nextInt(2), 1);
		}
	}
}

void Creeper::checkHurtTarget(Entity &target, float distance)
{
	int_t var1 = getSwellDir();
	if (var1 <= 0 && distance < 3.0f || var1 > 0 && distance < 7.0f)
	{
		if (swell == 0)
		{
			level.playSound(this, u"random.fuse", 1.0f, 0.5f);
		}

		setSwellDir(1);
		swell++;
		if (swell >= MAX_SWELL)
		{
			level.explode(this, x, y, z, 3.0f);
			remove();
		}

		holdGround = true;
	}
	else
	{
		setSwellDir(-1);
		swell--;
		if (swell < 0)
		{
			swell = 0;
		}
	}
}

float Creeper::getSwelling(float partialTick)
{
	return (oldSwell + (swell - oldSwell) * partialTick) / 28.0f;
}

int_t Creeper::getSwellDir()
{
	// TODO: return entityData.getByte(16); when SynchedEntityData is implemented
	return swellDir;
}

void Creeper::setSwellDir(int_t dir)
{
	// TODO: entityData.set(16, (byte)dir); when SynchedEntityData is implemented
	swellDir = dir;
}

int_t Creeper::getDeathLoot()
{
	if (Items::sulphur != nullptr)
		return Items::sulphur->getShiftedIndex();
	return 0;
}
