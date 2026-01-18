#pragma once

#include "world/entity/animal/Animal.h"
#include "nbt/CompoundTag.h"

class Level;

// Alpha 1.2.6 Chicken
// Reference: newb12/net/minecraft/world/entity/animal/Chicken.java
class Chicken : public Animal
{
public:
	bool sheared = false;  // Unused field (leftover from codebase)
	float flap = 0.0f;
	float flapSpeed = 0.0f;
	float oFlapSpeed = 0.0f;
	float oFlap = 0.0f;
	float flapping = 1.0f;
	int_t eggTime = 0;

public:
	Chicken(Level &level);

	virtual void aiStep() override;
	virtual void causeFallDamage(float distance) override;

	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;
	virtual int_t getDeathLoot() override;
};
