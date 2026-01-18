#pragma once

#include "world/entity/animal/Animal.h"
#include "nbt/CompoundTag.h"

class Level;
class Player;

// Alpha 1.2.6 Cow
// Reference: newb12/net/minecraft/world/entity/animal/Cow.java
class Cow : public Animal
{
public:
	Cow(Level &level);

	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;
	virtual float getSoundVolume() override;
	virtual int_t getDeathLoot() override;

public:
	virtual bool interact(Player &player) override;
};
