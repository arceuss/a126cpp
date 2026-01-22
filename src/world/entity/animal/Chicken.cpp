#include "world/entity/animal/Chicken.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/entity/item/EntityItem.h"
#include "nbt/CompoundTag.h"
#include "util/Memory.h"

Chicken::Chicken(Level &level) : Animal(level)
{
	textureName = u"/mob/chicken.png";
	setSize(0.3f, 0.4f);
	health = 4;
	eggTime = random.nextInt(6000) + 6000;
}

void Chicken::aiStep()
{
	Animal::aiStep();
	oFlap = flap;
	oFlapSpeed = flapSpeed;
	flapSpeed = (float)(flapSpeed + (onGround ? -1 : 4) * 0.3);
	if (flapSpeed < 0.0f)
	{
		flapSpeed = 0.0f;
	}

	if (flapSpeed > 1.0f)
	{
		flapSpeed = 1.0f;
	}

	if (!onGround && flapping < 1.0f)
	{
		flapping = 1.0f;
	}

	flapping = (float)(flapping * 0.9);
	if (!onGround && yd < 0.0)
	{
		yd *= 0.6;
	}

	flap = flap + flapping * 2.0f;
	if (!level.isOnline && --eggTime <= 0)
	{
		level.playSound(this, u"mob.chickenplop", 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
		// TODO: spawnAtLocation - create EntityItem with egg
		if (Items::egg != nullptr)
		{
			ItemStack stack(Items::egg->getShiftedIndex(), 1);
			auto itemEntity = std::make_shared<EntityItem>(level, x, y, z, stack);
			itemEntity->throwTime = 10;
			level.addEntity(itemEntity);
		}
		eggTime = random.nextInt(6000) + 6000;
	}
}

void Chicken::causeFallDamage(float distance)
{
	// Chickens don't take fall damage
}

void Chicken::addAdditionalSaveData(CompoundTag &tag)
{
	Animal::addAdditionalSaveData(tag);
}

void Chicken::readAdditionalSaveData(CompoundTag &tag)
{
	Animal::readAdditionalSaveData(tag);
}

jstring Chicken::getAmbientSound()
{
	return u"mob.chicken";
}

jstring Chicken::getHurtSound()
{
	return u"mob.chickenhurt";
}

jstring Chicken::getDeathSound()
{
	return u"mob.chickenhurt";
}

int_t Chicken::getDeathLoot()
{
	if (Items::feather != nullptr)
		return Items::feather->getShiftedIndex();
	return 0;
}
