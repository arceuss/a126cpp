#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"
#include "world/entity/player/Player.h"

class Level;

// Beta 1.2: FishingHook.java
class FishingHook : public Entity
{
private:
	int_t xTile = -1;
	int_t yTile = -1;
	int_t zTile = -1;
	int_t lastTile = 0;
	bool inGround = false;
	int_t life = 0;
	int_t flightTime = 0;
	int_t nibble = 0;
	int_t lSteps = 0;
	double lx = 0.0;
	double ly = 0.0;
	double lz = 0.0;
	double lyr = 0.0;
	double lxr = 0.0;
	double lxd = 0.0;
	double lyd = 0.0;
	double lzd = 0.0;

public:
	int_t shakeTime = 0;
	Player *owner = nullptr;
	Entity *hookedIn = nullptr;

public:
	FishingHook(Level &level);
	FishingHook(Level &level, double x, double y, double z);
	FishingHook(Level &level, Player &owner);

protected:
	virtual void defineSynchedData();

public:
	void shoot(double x, double y, double z, float speed, float inaccuracy);

public:
	virtual void lerpTo(double x, double y, double z, float yRot, float xRot, int steps) override;
	void lerpMotion(double x, double y, double z);

public:
	virtual void tick() override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	float getShadowHeightOffs();

	// Beta: retrieve() - retrieves the hook and returns damage amount (FishingHook.java:337-370)
	int_t retrieve();
};
