#pragma once

#include "client/particle/Particle.h"

// Beta 1.2: BubbleParticle.java - bubble particle for underwater
class BubbleParticle : public Particle
{
public:
	BubbleParticle(Level &level, double x, double y, double z, double xa, double ya, double za);
	
	virtual void tick() override;
};
