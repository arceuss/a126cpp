#pragma once

#include "world/entity/monster/Monster.h"

class Level;

// Alpha 1.2.6 Giant
// Reference: newb12/net/minecraft/world/entity/monster/Giant.java
class Giant : public Monster
{
public:
	Giant(Level &level);

protected:
	virtual float getWalkTargetValue(int_t x, int_t y, int_t z) override;
};
