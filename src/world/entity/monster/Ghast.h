#pragma once

#include "world/entity/FlyingMob.h"

class Level;
class Entity;

// Alpha 1.2.6 Ghast
// Reference: newb12/net/minecraft/world/entity/monster/Ghast.java
class Ghast : public FlyingMob
{
public:
	int_t floatDuration = 0;
	double xTarget = 0.0;
	double yTarget = 0.0;
	double zTarget = 0.0;
	int_t oCharge = 0;
	int_t charge = 0;

private:
	std::shared_ptr<Entity> target = nullptr;
	int_t retargetTime = 0;

public:
	Ghast(Level &level);

protected:
	virtual void updateAi() override;

private:
	bool canReach(double x, double y, double z, double distance);

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

protected:
	virtual void checkHurtTarget(Entity &target, float distance);

protected:
	virtual int_t getDeathLoot() override;

protected:
	virtual float getSoundVolume() override;

public:
	virtual bool canSpawn() override;

public:
	virtual int_t getMaxSpawnClusterSize() override;
};
