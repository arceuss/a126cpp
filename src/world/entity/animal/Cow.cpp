#include "world/entity/animal/Cow.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "nbt/CompoundTag.h"

Cow::Cow(Level &level) : Animal(level)
{
	textureName = u"/mob/cow.png";
	setSize(0.9f, 1.3f);
}

void Cow::addAdditionalSaveData(CompoundTag &tag)
{
	Animal::addAdditionalSaveData(tag);
}

void Cow::readAdditionalSaveData(CompoundTag &tag)
{
	Animal::readAdditionalSaveData(tag);
}

jstring Cow::getAmbientSound()
{
	return u"mob.cow";
}

jstring Cow::getHurtSound()
{
	return u"mob.cowhurt";
}

jstring Cow::getDeathSound()
{
	return u"mob.cowhurt";
}

float Cow::getSoundVolume()
{
	return 0.4f;
}


int_t Cow::getDeathLoot()
{
	if (Items::leather != nullptr)
		return Items::leather->getShiftedIndex();
	return 0;
}

bool Cow::interact(Player &player)
{
	ItemStack *selected = player.inventory.getSelected();
	if (selected != nullptr && Items::bucketEmpty != nullptr && selected->itemID == Items::bucketEmpty->getShiftedIndex())
	{
		if (Items::bucketMilk != nullptr)
		{
			ItemStack milkStack(Items::bucketMilk->getShiftedIndex(), 1);
			player.inventory.setItem(player.inventory.currentItem, milkStack);
			return true;
		}
	}
	return false;
}
