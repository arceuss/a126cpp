#pragma once

#include "world/entity/monster/Zombie.h"
#include "nbt/CompoundTag.h"
#include "world/item/ItemStack.h"

class Level;
class Entity;

// Alpha 1.2.6 PigZombie
// Reference: newb12/net/minecraft/world/entity/monster/PigZombie.java
class PigZombie : public Zombie
{
private:
	int_t angerTime = 0;
	int_t playAngrySoundIn = 0;
	static ItemStack sword;

public:
	PigZombie(Level &level);

public:
	virtual void tick() override;

public:
	virtual bool canSpawn() override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

protected:
	virtual std::shared_ptr<Entity> findAttackTarget() override;

public:
	virtual void aiStep() override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;

private:
	void alert(Entity *source);

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

protected:
	virtual int_t getDeathLoot() override;

public:
	virtual ItemStack *getCarriedItem() override;
};
