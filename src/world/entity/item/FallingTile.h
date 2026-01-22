#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"

class Level;

// Alpha 1.2.6 FallingTile
// Reference: newb12/net/minecraft/world/entity/item/FallingTile.java
class FallingTile : public Entity
{
public:
	int_t tile = 0;
	int_t time = 0;

public:
	FallingTile(Level &level);
	FallingTile(Level &level, double x, double y, double z, int_t tile);

protected:
	virtual void defineSynchedData();

public:
	virtual bool isPickable() override;

public:
	virtual void tick() override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual float getShadowHeightOffs();

public:
	Level &getLevel() { return level; }
};
