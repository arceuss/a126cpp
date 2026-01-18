#pragma once

#include "client/particle/Particle.h"

#include "java/Type.h"
#include <memory>

class Level;
class Entity;
class Tesselator;

class TakeAnimationParticle : public Particle
{
private:
	std::shared_ptr<Entity> item;
	std::shared_ptr<Entity> target;
	int_t life = 0;
	int_t lifeTime = 0;
	float yOffs;

public:
	// Beta: TakeAnimationParticle(Level level, Entity item, Entity target, float yOffs) (TakeAnimationParticle.java:17)
	TakeAnimationParticle(Level &level, std::shared_ptr<Entity> item, std::shared_ptr<Entity> target, float yOffs);

	virtual void tick() override;
	virtual void render(Tesselator &t, float a, float xa, float ya, float za, float xa2, float za2) override;
	virtual int_t getParticleTexture() const override;
};
