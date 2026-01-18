#pragma once

#include "world/entity/Mob.h"
#include "nbt/CompoundTag.h"
#include "world/entity/player/Player.h"

class Level;

// Alpha 1.2.6 Slime
// Reference: newb12/net/minecraft/world/entity/monster/Slime.java
class Slime : public Mob
{
public:
	float squish = 0.0f;
	float oSquish = 0.0f;
	int_t jumpDelay = 0;
	int_t size = 1;

public:
	Slime(Level &level);

public:
	void setSize(int_t size);

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual void tick() override;

protected:
	virtual void updateAi() override;

public:
	virtual void remove();

public:
	virtual void playerTouch(Player &player) override;

protected:
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

protected:
	virtual int_t getDeathLoot() override;

public:
	virtual bool canSpawn() override;

protected:
	virtual float getSoundVolume() override;
};
