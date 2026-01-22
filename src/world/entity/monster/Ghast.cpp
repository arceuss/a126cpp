#include "world/entity/monster/Ghast.h"

#include "world/level/Level.h"
#include "world/entity/projectile/Fireball.h"
#include "world/item/Items.h"
#include "world/phys/AABB.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

Ghast::Ghast(Level &level) : FlyingMob(level)
{
	textureName = u"/mob/ghast.png";
	setSize(4.0f, 4.0f);
	fireImmune = true;
}

void Ghast::updateAi()
{
	if (level.difficulty == 0)
	{
		remove();
	}

	oCharge = charge;
	double var1 = xTarget - x;
	double var3 = yTarget - y;
	double var5 = zTarget - z;
	double var7 = Mth::sqrt(var1 * var1 + var3 * var3 + var5 * var5);
	if (var7 < 1.0 || var7 > 60.0)
	{
		xTarget = x + (random.nextFloat() * 2.0f - 1.0f) * 16.0f;
		yTarget = y + (random.nextFloat() * 2.0f - 1.0f) * 16.0f;
		zTarget = z + (random.nextFloat() * 2.0f - 1.0f) * 16.0f;
	}

	if (floatDuration-- <= 0)
	{
		floatDuration = floatDuration + random.nextInt(5) + 2;
		if (canReach(xTarget, yTarget, zTarget, var7))
		{
			xd += var1 / var7 * 0.1;
			yd += var3 / var7 * 0.1;
			zd += var5 / var7 * 0.1;
		}
		else
		{
			xTarget = x;
			yTarget = y;
			zTarget = z;
		}
	}

	if (target != nullptr && target->removed)
	{
		target = nullptr;
	}

	if (target == nullptr || retargetTime-- <= 0)
	{
		std::shared_ptr<Player> player = level.getNearestPlayer(*this, 100.0);
		if (player != nullptr)
		{
			target = std::static_pointer_cast<Entity>(player);
			retargetTime = 20;
		}
	}

	double var9 = 64.0;
	if (target != nullptr && target->distanceToSqr(*this) < var9 * var9)
	{
		double var11 = target->x - x;
		double var13 = target->bb.y0 + target->bbHeight / 2.0f - (y + bbHeight / 2.0f);
		double var15 = target->z - z;
		yBodyRot = yRot = -((float)atan2(var11, var15)) * 180.0f / (float)M_PI;
		if (canSee(*target))
		{
			if (charge == 10)
			{
				level.playSound(this, u"mob.ghast.charge", getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
			}

			charge++;
			if (charge == 20)
			{
				level.playSound(this, u"mob.ghast.fireball", getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
				Fireball *var17 = new Fireball(level, *this, var11, var13, var15);
				double var18 = 4.0;
				Vec3 *var20 = getViewVector(1.0f);
				var17->x = x + var20->x * var18;
				var17->y = y + bbHeight / 2.0f + 0.5;
				var17->z = z + var20->z * var18;
				level.addEntity(std::shared_ptr<Entity>(var17));
				charge = -40;
			}
		}
		else if (charge > 0)
		{
			charge--;
		}
	}
	else
	{
		yBodyRot = yRot = -((float)atan2(xd, zd)) * 180.0f / (float)M_PI;
		if (charge > 0)
		{
			charge--;
		}
	}

	textureName = charge > 10 ? u"/mob/ghast_fire.png" : u"/mob/ghast.png";
}

bool Ghast::canReach(double var1, double var3, double var5, double var7)
{
	double var9 = (xTarget - x) / var7;
	double var11 = (yTarget - y) / var7;
	double var13 = (zTarget - z) / var7;
	AABB var15 = *bb.copy();

	for (int_t var16 = 1; var16 < var7; var16++)
	{
		var15.move(var9, var11, var13);
		if (level.getCubes(*this, var15).size() > 0)
		{
			return false;
		}
	}

	return true;
}

jstring Ghast::getAmbientSound()
{
	return u"mob.ghast.moan";
}

jstring Ghast::getHurtSound()
{
	return u"mob.ghast.scream";
}

jstring Ghast::getDeathSound()
{
	return u"mob.ghast.death";
}

void Ghast::checkHurtTarget(Entity &target, float distance)
{
	// Ghast doesn't melee attack
}

int_t Ghast::getDeathLoot()
{
	if (Items::sulphur != nullptr)
		return Items::sulphur->getShiftedIndex();
	return 0;
}

float Ghast::getSoundVolume()
{
	return 10.0f;
}

bool Ghast::canSpawn()
{
	return random.nextInt(20) == 0 && FlyingMob::canSpawn() && level.difficulty > 0;
}

int_t Ghast::getMaxSpawnClusterSize()
{
	return 1;
}
