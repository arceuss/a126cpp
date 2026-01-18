#pragma once

#include "client/particle/Particle.h"

// Beta 1.2: WaterDropParticle.java - base class for water-related particles
class WaterDropParticle : public Particle
{
public:
	WaterDropParticle(Level &level, double x, double y, double z);
	
	virtual void tick() override;
};
