#include "client/particle/WaterDropParticle.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/tile/FluidTile.h"
#include "util/Mth.h"

// Beta 1.2: WaterDropParticle.java - base class for water-related particles
WaterDropParticle::WaterDropParticle(Level &level, double x, double y, double z)
	: Particle(level, x, y, z, 0.0, 0.0, 0.0)
{
	// Beta: WaterDropParticle constructor (WaterDropParticle.java:10-22)
	xd *= 0.3f;
	yd = random.nextFloat() * 0.2f + 0.1f;
	zd *= 0.3f;
	rCol = 1.0f;
	gCol = 1.0f;
	bCol = 1.0f;
	tex = 19 + random.nextInt(4);  // Beta: 19 + random.nextInt(4) (WaterDropParticle.java:18)
	setSize(0.01f, 0.01f);
	gravity = 0.06f;
	lifetime = (int_t)(8.0 / (random.nextFloat() * 0.8 + 0.2));
}

void WaterDropParticle::tick()
{
	// Beta: WaterDropParticle.tick() (WaterDropParticle.java:30-59)
	xo = x;
	yo = y;
	zo = z;
	yd = yd - gravity;
	move(xd, yd, zd);
	xd *= 0.98f;
	yd *= 0.98f;
	zd *= 0.98f;
	
	if (lifetime-- <= 0)
	{
		remove();
		return;
	}
	
	if (onGround)
	{
		if (random.nextFloat() < 0.5f)
		{
			remove();
			return;
		}
		xd *= 0.7f;
		zd *= 0.7f;
	}
	
	// Beta: Check if particle is in liquid or solid (WaterDropParticle.java:52-58)
	const Material &m = level.getMaterial(Mth::floor(x), Mth::floor(y), Mth::floor(z));
	if (m.isLiquid() || m.isSolid())
	{
		double y0 = Mth::floor(y) + 1 - FluidTile::getHeight(level.getData(Mth::floor(x), Mth::floor(y), Mth::floor(z)));
		if (y < y0)
		{
			remove();
		}
	}
}
