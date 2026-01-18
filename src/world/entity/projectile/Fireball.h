#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"
#include "world/entity/Mob.h"

class Level;

// Alpha 1.2.6 Fireball
// Reference: newb12/net/minecraft/world/entity/projectile/Fireball.java
class Fireball : public Entity
{
private:
	int_t xTile = -1;
	int_t yTile = -1;
	int_t zTile = -1;
	int_t lastTile = 0;
	bool inGround = false;
	int_t life = 0;
	int_t flightTime = 0;

public:
	int_t shakeTime = 0;
	double xPower = 0.0;
	double yPower = 0.0;
	double zPower = 0.0;

private:
	Mob *owner = nullptr;

public:
	Fireball(Level &level);
	Fireball(Level &level, Mob &owner, double x, double y, double z);

protected:
	virtual void defineSynchedData();

public:
	virtual bool shouldRenderAtSqrDistance(double distance) override;

public:
	virtual void tick() override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual bool isPickable() override;

public:
	virtual float getPickRadius() override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;

public:
	virtual float getShadowHeightOffs();
};
