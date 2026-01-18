#pragma once

#include "world/entity/monster/Monster.h"

class Level;

// Alpha 1.2.6 Zombie
// Reference: newb12/net/minecraft/world/entity/monster/Zombie.java
class Zombie : public Monster
{
public:
	Zombie(Level &level);

public:
	virtual void aiStep() override;

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

protected:
	virtual int_t getDeathLoot() override;
};
