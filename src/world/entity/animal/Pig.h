#pragma once

#include "world/entity/animal/Animal.h"
#include "nbt/CompoundTag.h"

class Level;
class Player;

// Alpha 1.2.6 Pig
// Reference: newb12/net/minecraft/world/entity/animal/Pig.java
class Pig : public Animal
{
private:
	static constexpr int_t DATA_SADDLE_ID = 16;
	byte_t entityDataSaddle = 0;  // TODO: Replace with proper SynchedEntityData

public:
	Pig(Level &level);

protected:
	virtual void defineSynchedData() override;

	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;
	virtual int_t getDeathLoot() override;

public:
	virtual bool interact(Player &player) override;

	bool hasSaddle();
	void setSaddle(bool value);
};
