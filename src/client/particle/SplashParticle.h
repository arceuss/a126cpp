#pragma once

#include "client/particle/WaterDropParticle.h"

// Beta 1.2: SplashParticle.java - splash particle for water entry
class SplashParticle : public WaterDropParticle
{
public:
	SplashParticle(Level &level, double x, double y, double z, double xa, double ya, double za);
};
