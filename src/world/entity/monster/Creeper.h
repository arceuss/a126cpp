#pragma once

#include "world/entity/monster/Monster.h"
#include "nbt/CompoundTag.h"

class Level;
class Entity;

// Alpha 1.2.6 Creeper
// Reference: newb12/net/minecraft/world/entity/monster/Creeper.java
class Creeper : public Monster
{
private:
	static constexpr int_t DATA_SWELL_DIR = 16;
	static constexpr int_t MAX_SWELL = 30;

public:
	int_t swell = 0;
	int_t oldSwell = 0;

private:
	// TODO: Replace with SynchedEntityData when implemented
	int_t swellDir = -1;

public:
	Creeper(Level &level);

protected:
	virtual void defineSynchedData() override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual void tick() override;

protected:
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

public:
	virtual void die(Entity *source);

protected:
	virtual void checkHurtTarget(Entity &target, float distance) override;

public:
	float getSwelling(float partialTick);

private:
	int_t getSwellDir();
	void setSwellDir(int_t dir);

protected:
	virtual int_t getDeathLoot() override;
};
