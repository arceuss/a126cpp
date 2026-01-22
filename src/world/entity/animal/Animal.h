#pragma once

#include "world/entity/monster/PathfinderMob.h"

#include "nbt/CompoundTag.h"

class Level;

// Alpha 1.2.6 Animal - base class for animals (Pig, Sheep, Cow, Chicken)
// Reference: newb12/net/minecraft/world/entity/animal/Animal.java
class Animal : public PathfinderMob
{
public:
	Animal(Level &level);

protected:
	virtual float getWalkTargetValue(int_t x, int_t y, int_t z) override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;
	virtual bool canSpawn() override;

protected:
	virtual int_t getAmbientSoundInterval() override;
};
