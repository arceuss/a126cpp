#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"

class Level;

// Alpha 1.2.6 PrimedTnt
// Reference: newb12/net/minecraft/world/entity/item/PrimedTnt.java
class PrimedTnt : public Entity
{
public:
	int_t life = 0;

public:
	PrimedTnt(Level &level);
	PrimedTnt(Level &level, double x, double y, double z);

protected:
	virtual void defineSynchedData();

public:
	virtual bool isPickable() override;

public:
	virtual void tick() override;

private:
	void explode();

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual float getShadowHeightOffs();
};
