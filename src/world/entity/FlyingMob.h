#pragma once

#include "world/entity/Mob.h"

class Level;

// Alpha 1.2.6 FlyingMob - base class for flying mobs (e.g., Ghast)
// Reference: newb12/net/minecraft/world/entity/FlyingMob.java
class FlyingMob : public Mob
{
public:
	FlyingMob(Level &level);

protected:
	virtual void causeFallDamage(float distance) override;
	virtual void travel(float x, float z) override;
	virtual bool onLadder() override;
};
