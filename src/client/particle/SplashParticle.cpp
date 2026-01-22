#include "client/particle/SplashParticle.h"

#include "world/level/Level.h"

// Beta 1.2: SplashParticle.java - splash particle for water entry
SplashParticle::SplashParticle(Level &level, double x, double y, double z, double xa, double ya, double za)
	: WaterDropParticle(level, x, y, z)
{
	// Beta: SplashParticle constructor (SplashParticle.java:6-15)
	gravity = 0.04f;
	tex++;  // Beta: this.tex++ (SplashParticle.java:9)
	if (ya == 0.0 && (xa != 0.0 || za != 0.0))
	{
		xd = xa;
		yd = ya + 0.1;
		zd = za;
	}
}
