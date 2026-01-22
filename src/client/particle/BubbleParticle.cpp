#include "client/particle/BubbleParticle.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "util/Mth.h"

// Beta 1.2: BubbleParticle.java - bubble particle for underwater
BubbleParticle::BubbleParticle(Level &level, double x, double y, double z, double xa, double ya, double za)
	: Particle(level, x, y, z, xa, ya, za)
{
	// Beta: BubbleParticle constructor (BubbleParticle.java:8-20)
	rCol = 1.0f;
	gCol = 1.0f;
	bCol = 1.0f;
	tex = 32;  // Beta: this.tex = 32 (BubbleParticle.java:13)
	setSize(0.02f, 0.02f);
	size = size * (random.nextFloat() * 0.6f + 0.2f);
	xd = xa * 0.2f + (float)((random.nextFloat() * 2.0 - 1.0) * 0.02f);
	yd = ya * 0.2f + (float)((random.nextFloat() * 2.0 - 1.0) * 0.02f);
	zd = za * 0.2f + (float)((random.nextFloat() * 2.0 - 1.0) * 0.02f);
	lifetime = (int_t)(8.0 / (random.nextFloat() * 0.8 + 0.2));
}

void BubbleParticle::tick()
{
	// Beta: BubbleParticle.tick() (BubbleParticle.java:23-39)
	xo = x;
	yo = y;
	zo = z;
	yd += 0.002;  // Beta: this.yd += 0.002 (BubbleParticle.java:27)
	move(xd, yd, zd);
	xd *= 0.85f;
	yd *= 0.85f;
	zd *= 0.85f;
	
	// Beta: Remove if not in water (BubbleParticle.java:32-34)
	const Material &mat = level.getMaterial(Mth::floor(x), Mth::floor(y), Mth::floor(z));
	const Material &waterMat = Material::water;  // Cast LiquidMaterial to Material& for comparison
	if (&mat != &waterMat)
	{
		remove();
		return;
	}
	
	if (lifetime-- <= 0)
	{
		remove();
	}
}
