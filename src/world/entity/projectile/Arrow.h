#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"
#include "world/entity/Mob.h"
#include "world/entity/player/Player.h"

class Level;

// Alpha 1.2.6 Arrow
// Reference: newb12/net/minecraft/world/entity/projectile/Arrow.java
class Arrow : public Entity
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
	Mob *owner = nullptr;

public:
	Arrow(Level &level);
	Arrow(Level &level, double x, double y, double z);
	Arrow(Level &level, Mob &owner);

protected:
	virtual void defineSynchedData();

public:
	void shoot(double x, double y, double z, float speed, float inaccuracy);

public:
	virtual void lerpMotion(double x, double y, double z);

public:
	virtual void tick() override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual void playerTouch(Player &player) override;

public:
	virtual float getShadowHeightOffs();
};
