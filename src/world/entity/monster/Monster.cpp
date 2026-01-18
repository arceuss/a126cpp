#include "world/entity/monster/Monster.h"
#include "world/level/Level.h"
#include "world/level/LightLayer.h"
#include "util/Mth.h"

// Beta 1.2 Monster
Monster::Monster(Level &level) : PathfinderMob(level)
{
	health = 20;  // Beta: Monster.health = 20
}

void Monster::aiStep()
{
	// Beta: Monster.aiStep() - increases noActionTime in bright light
	float brightness = getBrightness(1.0f);
	if (brightness > 0.5f)
	{
		noActionTime += 2;
	}
	
	PathfinderMob::aiStep();
}

void Monster::tick()
{
	// Beta: Monster.tick() - removes on peaceful difficulty
	PathfinderMob::tick();
	if (level.difficulty == 0)
	{
		remove();
	}
}

std::shared_ptr<Entity> Monster::findAttackTarget()
{
	// Beta: Monster.findAttackTarget() - finds nearest player within 16 blocks
	std::shared_ptr<Player> player = level.getNearestPlayer(*this, 16.0);
	if (player != nullptr && canSee(*player))
	{
		return std::static_pointer_cast<Entity>(player);
	}
	return nullptr;
}

bool Monster::hurt(Entity *source, int_t dmg)
{
	// Beta: Monster.hurt() - sets attackTarget when hurt
	if (PathfinderMob::hurt(source, dmg))
	{
		if (source != nullptr && source != this && rider.get() != source && riding.get() != source)
		{
			attackTarget = std::shared_ptr<Entity>(source, [](Entity*){});
		}
		return true;
	}
	return false;
}

void Monster::checkHurtTarget(Entity &target, float distance)
{
	// Beta: Monster.checkHurtTarget() - attacks if close enough
	if (distance < 2.5f && target.bb.y1 > bb.y0 && target.bb.y0 < bb.y1)
	{
		attackTime = 20;
		target.hurt(this, attackDamage);
	}
}

float Monster::getWalkTargetValue(int_t x, int_t y, int_t z)
{
	// Beta: Monster.getWalkTargetValue() - prefers darker areas
	return 0.5f - level.getBrightness(x, y, z);
}

bool Monster::canSpawn()
{
	// Beta: Monster.canSpawn() - requires low light level
	int_t x = Mth::floor(this->x);
	int_t y = Mth::floor(bb.y0);
	int_t z = Mth::floor(this->z);
	
	if (level.getBrightness(LightLayer::Sky, x, y, z) > random.nextInt(32))
	{
		return false;
	}
	
	int_t blockLight = level.getRawBrightness(x, y, z);
	return blockLight <= random.nextInt(8) && PathfinderMob::canSpawn();
}
