#include "world/entity/monster/Spider.h"

#include "world/level/Level.h"
#include "world/entity/player/Player.h"
#include "world/item/Items.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"

Spider::Spider(Level &level) : Monster(level)
{
	textureName = u"/mob/spider.png";
	setSize(1.4f, 0.9f);
	runSpeed = 0.8f;
}

double Spider::getRideHeight()
{
	// newb12: Spider.getRideHeight() - returns bbHeight * 0.75 - 0.5 (Spider.java:18-20)
	return bbHeight * 0.75 - 0.5;
}

std::shared_ptr<Entity> Spider::findAttackTarget()
{
	// newb12: Spider.findAttackTarget() - only attacks in darkness (Spider.java:22-31)
	float brightness = getBrightness(1.0f);
	if (brightness < 0.5f)
	{
		double distance = 16.0;
		std::shared_ptr<Player> player = level.getNearestPlayer(*this, distance);
		if (player != nullptr)
		{
			return std::static_pointer_cast<Entity>(player);
		}
	}
	return nullptr;
}

void Spider::checkHurtTarget(Entity &target, float distance)
{
	// newb12: Spider.checkHurtTarget() - special jumping attack logic (Spider.java:49-65)
	float brightness = getBrightness(1.0f);
	if (brightness > 0.5f && random.nextInt(100) == 0)
	{
		attackTarget = nullptr;
	}
	else
	{
		if (!(distance > 2.0f) || !(distance < 6.0f) || random.nextInt(10) != 0)
		{
			Monster::checkHurtTarget(target, distance);
		}
		else if (onGround)
		{
			double dx = target.x - x;
			double dz = target.z - z;
			float dist = Mth::sqrt(dx * dx + dz * dz);
			xd = dx / dist * 0.5 * 0.8f + xd * 0.2f;
			zd = dz / dist * 0.5 * 0.8f + zd * 0.2f;
			yd = 0.4f;
		}
	}
}

jstring Spider::getAmbientSound()
{
	// newb12: Spider.getAmbientSound() - returns "mob.spider" (Spider.java:34-36)
	return u"mob.spider";
}

jstring Spider::getHurtSound()
{
	// newb12: Spider.getHurtSound() - returns "mob.spider" (Spider.java:38-41)
	return u"mob.spider";
}

jstring Spider::getDeathSound()
{
	// newb12: Spider.getDeathSound() - returns "mob.spiderdeath" (Spider.java:43-46)
	return u"mob.spiderdeath";
}

int_t Spider::getDeathLoot()
{
	// newb12: Spider.getDeathLoot() - returns Item.string.id (Spider.java:78-80)
	if (Items::string != nullptr)
		return Items::string->getShiftedIndex();
	return 0;
}

bool Spider::onLadder()
{
	// newb12: Spider.onLadder() - returns horizontalCollision (Spider.java:83-85)
	return horizontalCollision;
}
