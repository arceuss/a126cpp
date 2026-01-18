#pragma once

#include "world/entity/monster/Monster.h"
#include "nbt/CompoundTag.h"
#include "world/item/ItemStack.h"

class Level;
class Entity;

// Alpha 1.2.6 Skeleton
// Reference: newb12/net/minecraft/world/entity/monster/Skeleton.java
class Skeleton : public Monster
{
private:
	static ItemStack bow;

public:
	Skeleton(Level &level);

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

public:
	virtual void aiStep() override;

protected:
	virtual void checkHurtTarget(Entity &target, float distance) override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

protected:
	virtual int_t getDeathLoot() override;

protected:
	virtual void dropDeathLoot() override;

public:
	virtual ItemStack *getCarriedItem() override;
};
