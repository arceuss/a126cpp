#include "world/entity/monster/Skeleton.h"

#include "world/level/Level.h"
#include "world/entity/projectile/Arrow.h"
#include "world/item/Items.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

ItemStack Skeleton::bow = ItemStack();

Skeleton::Skeleton(Level &level) : Monster(level)
{
	textureName = u"/mob/skeleton.png";
	if (bow.isEmpty() && Items::bow != nullptr)
	{
		bow = ItemStack(Items::bow->getShiftedIndex(), 1);
	}
}

jstring Skeleton::getAmbientSound()
{
	return u"mob.skeleton";
}

jstring Skeleton::getHurtSound()
{
	return u"mob.skeletonhurt";
}

jstring Skeleton::getDeathSound()
{
	return u"mob.skeletonhurt";
}

void Skeleton::aiStep()
{
	if (level.isDay())
	{
		float var1 = getBrightness(1.0f);
		if (var1 > 0.5f
			&& level.canSeeSky(Mth::floor(x), Mth::floor(y), Mth::floor(z))
			&& random.nextFloat() * 30.0f < (var1 - 0.4f) * 2.0f)
		{
			onFire = 300;
		}
	}

	Monster::aiStep();
}

void Skeleton::checkHurtTarget(Entity &target, float distance)
{
	if (distance < 10.0f)
	{
		double var3 = target.x - x;
		double var5 = target.z - z;
		if (attackTime == 0)
		{
			Arrow *var7 = new Arrow(level, *this);
			var7->y++;
			double var8 = target.y - 0.2f - var7->y;
			float var10 = Mth::sqrt(var3 * var3 + var5 * var5) * 0.2f;
			level.playSound(this, u"random.bow", 1.0f, 1.0f / (random.nextFloat() * 0.4f + 0.8f));
			level.addEntity(std::shared_ptr<Entity>(var7));
			var7->shoot(var3, var8 + var10, var5, 0.6f, 12.0f);
			attackTime = 30;
		}

		yRot = (float)(atan2(var5, var3) * 180.0 / M_PI) - 90.0f;
		holdGround = true;
	}
}

void Skeleton::addAdditionalSaveData(CompoundTag &tag)
{
	Monster::addAdditionalSaveData(tag);
}

void Skeleton::readAdditionalSaveData(CompoundTag &tag)
{
	Monster::readAdditionalSaveData(tag);
}

int_t Skeleton::getDeathLoot()
{
	if (Items::arrow != nullptr)
		return Items::arrow->getShiftedIndex();
	return 0;
}

void Skeleton::dropDeathLoot()
{
	int_t var1 = random.nextInt(3);

	for (int_t var2 = 0; var2 < var1; var2++)
	{
		if (Items::arrow != nullptr)
			spawnAtLocation(Items::arrow->getShiftedIndex(), 1);
	}

	var1 = random.nextInt(3);

	for (int_t var4 = 0; var4 < var1; var4++)
	{
		if (Items::bone != nullptr)
			spawnAtLocation(Items::bone->getShiftedIndex(), 1);
	}
}

ItemStack *Skeleton::getCarriedItem()
{
	// Alpha 1.2.6: Skeleton.getCarriedItem() - returns bow from DataWatcher (for multiplayer) or static bow (for singleplayer)
	// In multiplayer, the carried item is synchronized via DataWatcher (data ID 1)
	// Reference: Alpha 1.2.6 EntityLiving/EntityMobs likely uses data ID 1 for held item
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
					// Store in static bow for now (since we return a pointer)
					// TODO: Consider per-instance storage for multiplayer skeletons
					bow = carriedItem;
					return &bow;
				}
			}
			catch (...)
			{
				// DataWatcher doesn't have the item yet, fall through to static bow
			}
		}
	}
	
	// Fall back to static bow (for singleplayer or if DataWatcher doesn't have it)
	return bow.isEmpty() ? nullptr : &bow;
}
