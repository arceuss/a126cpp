#include "world/entity/PrimedTnt.h"

#include "world/level/Level.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

PrimedTnt::PrimedTnt(Level &level) : Entity(level)
{
	blocksBuilding = true;
	setSize(0.98f, 0.98f);
	heightOffset = bbHeight / 2.0f;
}

PrimedTnt::PrimedTnt(Level &level, double x, double y, double z) : PrimedTnt(level)
{
	setPos(x, y, z);
	float var8 = (float)(Math::random() * M_PI * 2.0);
	xd = -Mth::sin(var8 * (float)M_PI / 180.0f) * 0.02f;
	yd = 0.2f;
	zd = -Mth::cos(var8 * (float)M_PI / 180.0f) * 0.02f;
	makeStepSound = false;
	life = 80;
	xo = x;
	yo = y;
	zo = z;
}

void PrimedTnt::defineSynchedData()
{
}

bool PrimedTnt::isPickable()
{
	return !removed;
}

void PrimedTnt::tick()
{
	xo = x;
	yo = y;
	zo = z;
	yd -= 0.04f;
	move(xd, yd, zd);
	xd *= 0.98f;
	yd *= 0.98f;
	zd *= 0.98f;
	if (onGround)
	{
		xd *= 0.7f;
		zd *= 0.7f;
		yd *= -0.5;
	}

	if (life-- <= 0)
	{
		remove();
		explode();
	}
	else
	{
		level.addParticle(u"smoke", x, y + 0.5, z, 0.0, 0.0, 0.0);
	}
}

void PrimedTnt::explode()
{
	float var1 = 4.0f;
	level.explode(nullptr, x, y, z, var1);
}

void PrimedTnt::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putByte(u"Fuse", (byte_t)life);
}

void PrimedTnt::readAdditionalSaveData(CompoundTag &tag)
{
	life = tag.getByte(u"Fuse");
}

float PrimedTnt::getShadowHeightOffs()
{
	return 0.0f;
}
