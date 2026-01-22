#pragma once

#include "world/entity/monster/PathfinderMob.h"
#include "world/entity/player/Player.h"
#include "world/level/LightLayer.h"
#include <memory>

// Beta 1.2 Monster - base class for hostile mobs
// Reference: deobfb12/minecraft/src/net/minecraft/world/entity/monster/Monster.java
class Monster : public PathfinderMob
{
protected:
	int_t attackDamage = 2;  // Beta: Monster.attackDamage

public:
	Monster(Level &level);

	virtual void aiStep() override;
	virtual void tick() override;

protected:
	virtual std::shared_ptr<Entity> findAttackTarget() override;
	virtual void checkHurtTarget(Entity &target, float distance) override;
	virtual float getWalkTargetValue(int_t x, int_t y, int_t z) override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;
	virtual bool canSpawn() override;
};
