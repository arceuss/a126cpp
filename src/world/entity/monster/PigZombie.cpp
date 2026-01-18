#include "world/entity/monster/PigZombie.h"

#include "world/level/Level.h"
#include "world/entity/player/Player.h"
#include "world/item/Items.h"
#include "nbt/CompoundTag.h"
#include <vector>

ItemStack PigZombie::sword = ItemStack();

PigZombie::PigZombie(Level &level) : Zombie(level)
{
	textureName = u"/mob/pigzombie.png";
	runSpeed = 0.5f;
	attackDamage = 5;
	fireImmune = true;
	if (sword.isEmpty() && Items::swordGold != nullptr)
	{
		sword = ItemStack(Items::swordGold->getShiftedIndex(), 1);
	}
}

void PigZombie::tick()
{
	runSpeed = attackTarget != nullptr ? 0.95f : 0.5f;
	if (playAngrySoundIn > 0 && --playAngrySoundIn == 0)
	{
		level.playSound(this, u"mob.zombiepig.zpigangry", getSoundVolume() * 2.0f, ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 1.8f);
	}

	Zombie::tick();
}

bool PigZombie::canSpawn()
{
	return level.difficulty > 0
		&& level.isUnobstructed(bb)
		&& level.getCubes(*this, bb).size() == 0
		&& !level.containsAnyLiquid(bb);
}

void PigZombie::addAdditionalSaveData(CompoundTag &tag)
{
	Zombie::addAdditionalSaveData(tag);
	tag.putShort(u"Anger", (short_t)angerTime);
}

void PigZombie::readAdditionalSaveData(CompoundTag &tag)
{
	Zombie::readAdditionalSaveData(tag);
	angerTime = tag.getShort(u"Anger");
}

std::shared_ptr<Entity> PigZombie::findAttackTarget()
{
	return angerTime == 0 ? nullptr : Zombie::findAttackTarget();
}

void PigZombie::aiStep()
{
	Zombie::aiStep();
}

bool PigZombie::hurt(Entity *source, int_t dmg)
{
	if (source != nullptr && dynamic_cast<Player*>(source) != nullptr)
	{
		AABB expanded = *bb.grow(32.0, 32.0, 32.0);
		std::vector<std::shared_ptr<Entity>> var3 = level.getEntities(this, expanded);

		for (size_t var4 = 0; var4 < var3.size(); var4++)
		{
			std::shared_ptr<Entity> var5 = var3[var4];
			PigZombie *var6 = dynamic_cast<PigZombie*>(var5.get());
			if (var6 != nullptr)
			{
				var6->alert(source);
			}
		}

		alert(source);
	}

	return Zombie::hurt(source, dmg);
}

void PigZombie::alert(Entity *source)
{
	attackTarget = std::shared_ptr<Entity>(source, [](Entity*){});
	angerTime = 400 + random.nextInt(400);
	playAngrySoundIn = random.nextInt(40);
}

jstring PigZombie::getAmbientSound()
{
	return u"mob.zombiepig.zpig";
}

jstring PigZombie::getHurtSound()
{
	return u"mob.zombiepig.zpighurt";
}

jstring PigZombie::getDeathSound()
{
	return u"mob.zombiepig.zpigdeath";
}

int_t PigZombie::getDeathLoot()
{
	if (Items::porkCooked != nullptr)
		return Items::porkCooked->getShiftedIndex();
	return 0;
}

ItemStack *PigZombie::getCarriedItem()
{
	// Alpha 1.2.6: PigZombie.getCarriedItem() - returns sword from DataWatcher (for multiplayer) or static sword (for singleplayer)
	// In multiplayer, the carried item is synchronized via DataWatcher (data ID 1)
	static constexpr int_t DATA_CARRIED_ITEM_ID = 1;
	
	// For client-side entities (spawned from network), read from DataWatcher
	if (field_9343_G || interpolateOnly)
	{
		if (dataWatcher.hasWatchableObject(DATA_CARRIED_ITEM_ID))
		{
			try
			{
				ItemStack carriedItem = dataWatcher.getWatchableObjectItemStack(DATA_CARRIED_ITEM_ID);
				if (!carriedItem.isEmpty())
				{
					// Store in static sword for now (since we return a pointer)
					// TODO: Consider per-instance storage for multiplayer pig zombies
					sword = carriedItem;
					return &sword;
				}
			}
			catch (...)
			{
				// DataWatcher doesn't have the item yet, fall through to static sword
			}
		}
	}
	
	// Fall back to static sword (for singleplayer or if DataWatcher doesn't have it)
	return sword.isEmpty() ? nullptr : &sword;
}
