#pragma once

#include "world/entity/monster/Monster.h"
#include "nbt/CompoundTag.h"

class Level;
class Entity;

// Alpha 1.2.6 Spider
// Reference: newb12/net/minecraft/world/entity/monster/Spider.java
class Spider : public Monster
{
public:
	Spider(Level &level);

public:
	virtual double getRideHeight() override;

protected:
	virtual std::shared_ptr<Entity> findAttackTarget() override;
	virtual void checkHurtTarget(Entity &target, float distance) override;

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;
	virtual int_t getDeathLoot() override;

public:
	virtual bool onLadder() override;
};
