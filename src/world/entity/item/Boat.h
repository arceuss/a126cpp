#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"
#include "world/phys/AABB.h"

class Level;
class Player;

// Alpha 1.2.6 Boat
// Reference: newb12/net/minecraft/world/entity/item/Boat.java
class Boat : public Entity
{
public:
	int_t damage = 0;
	int_t hurtTime = 0;
	int_t hurtDir = 1;

private:
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
	Boat(Level &level);
	Boat(Level &level, double x, double y, double z);

protected:
	virtual void defineSynchedData();

public:
	virtual AABB *getCollideAgainstBox(Entity &entity) override;
	virtual AABB *getCollideBox() override;
	virtual bool isPushable() override;

public:
	virtual double getRideHeight() override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;
	virtual void animateHurt() override;
	virtual bool isPickable() override;

public:
	virtual void lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps) override;
	virtual void lerpMotion(double x, double y, double z);

public:
	virtual void tick() override;
	virtual void positionRider();

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual float getShadowHeightOffs();

public:
	jstring getName() { return u"Boat"; }
	void setChanged() {}

public:
	virtual bool interact(Player &player) override;
};
