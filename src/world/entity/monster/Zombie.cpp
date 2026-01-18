#include "world/entity/monster/Zombie.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "util/Mth.h"

Zombie::Zombie(Level &level) : Monster(level)
{
	textureName = u"/mob/zombie.png";
	runSpeed = 0.5f;
	attackDamage = 5;
}

void Zombie::aiStep()
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

jstring Zombie::getAmbientSound()
{
	return u"mob.zombie";
}

jstring Zombie::getHurtSound()
{
	return u"mob.zombiehurt";
}

jstring Zombie::getDeathSound()
{
	return u"mob.zombiedeath";
}

int_t Zombie::getDeathLoot()
{
	if (Items::feather != nullptr)
		return Items::feather->getShiftedIndex();
	return 0;
}
