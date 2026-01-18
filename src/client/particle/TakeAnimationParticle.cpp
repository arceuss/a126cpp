#include "client/particle/TakeAnimationParticle.h"

#include "world/level/Level.h"
#include "world/entity/Entity.h"
#include "client/renderer/Tesselator.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"

#include "util/Mth.h"

#include "pc/OpenGL.h"

TakeAnimationParticle::TakeAnimationParticle(Level &level, std::shared_ptr<Entity> item, std::shared_ptr<Entity> target, float yOffs)
	: Particle(level, item->x, item->y, item->z, item->xd, item->yd, item->zd), item(item), target(target), yOffs(yOffs)
{
	// Beta: TakeAnimationParticle constructor (TakeAnimationParticle.java:17-22)
	lifeTime = 3;
	life = 0;
}

void TakeAnimationParticle::tick()
{
	// Beta: TakeAnimationParticle.tick() (TakeAnimationParticle.java:50-54)
	life++;
	if (life >= lifeTime)
	{
		remove();
	}
}

int_t TakeAnimationParticle::getParticleTexture() const
{
	// Beta: TakeAnimationParticle.getParticleTexture() returns 3 (TakeAnimationParticle.java:58-60)
	return 3;  // ENTITY_PARTICLE_TEXTURE
}

void TakeAnimationParticle::render(Tesselator &t, float a, float xa, float ya, float za, float xa2, float za2)
{
	// Beta: TakeAnimationParticle.render() (TakeAnimationParticle.java:26-46)
	float time = (float)(life + a) / (float)lifeTime;
	time *= time;
	
	double xo = item->x;
	double yo = item->y;
	double zo = item->z;
	double xt = target->xo + (target->x - target->xo) * a;
	double yt = target->yo + (target->y - target->yo) * a + yOffs;
	double zt = target->zo + (target->z - target->zo) * a;
	
	double xx = xo + (xt - xo) * time;
	double yy = yo + (yt - yo) * time;
	double zz = zo + (zt - zo) * time;
	
	int_t xTile = Mth::floor(xx);
	int_t yTile = Mth::floor(yy + heightOffset / 2.0f);
	int_t zTile = Mth::floor(zz);
	
	float br = level.getBrightness(xTile, yTile, zTile);
	
	xx -= Particle::xOff;
	yy -= Particle::yOff;
	zz -= Particle::zOff;
	
	glColor4f(br, br, br, 1.0f);
	EntityRenderDispatcher::instance.render(*item, (float)xx, (float)yy, (float)zz, item->yRot, a);
}
