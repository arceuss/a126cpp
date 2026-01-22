#pragma once

#include "world/entity/animal/Animal.h"
#include "nbt/CompoundTag.h"

class Level;
class Entity;

// Alpha 1.2.6 Sheep
// Reference: newb12/net/minecraft/world/entity/animal/Sheep.java
// Note: Alpha doesn't have colored wool - sheep are always white
class Sheep : public Animal
{
private:
	static constexpr int_t DATA_WOOL_ID = 16;

public:
	Sheep(Level &level);

protected:
	virtual void defineSynchedData() override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;

	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

protected:
	virtual jstring getAmbientSound() override;
	virtual jstring getHurtSound() override;
	virtual jstring getDeathSound() override;

public:
	bool isSheared();
	void setSheared(bool value);
};
