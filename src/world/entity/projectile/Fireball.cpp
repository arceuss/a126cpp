#include "world/entity/projectile/Fireball.h"

#include "world/level/Level.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

// Simple Gaussian approximation using Box-Muller transform
static float nextGaussian(Random &random)
{
	// Box-Muller transform
	float u1 = random.nextFloat();
	float u2 = random.nextFloat();
	if (u1 == 0.0f) u1 = 0.0001f; // Avoid log(0)
	return sqrt(-2.0f * log(u1)) * cos(2.0f * M_PI * u2);
}

Fireball::Fireball(Level &level) : Entity(level)
{
	setSize(1.0f, 1.0f);
}

void Fireball::defineSynchedData()
{
}

bool Fireball::shouldRenderAtSqrDistance(double var1)
{
	double var3 = bb.getSize() * 4.0;
	var3 *= 64.0;
	return var1 < var3 * var3;
}

Fireball::Fireball(Level &level, Mob &owner, double var3, double var5, double var7) : Entity(level)
{
	this->owner = &owner;
	setSize(1.0f, 1.0f);
	moveTo(owner.x, owner.y, owner.z, owner.yRot, owner.xRot);
	setPos(x, y, z);
	heightOffset = 0.0f;
	xd = yd = zd = 0.0;
	var3 += nextGaussian(random) * 0.4;
	var5 += nextGaussian(random) * 0.4;
	var7 += nextGaussian(random) * 0.4;
	double var9 = Mth::sqrt(var3 * var3 + var5 * var5 + var7 * var7);
	xPower = var3 / var9 * 0.1;
	yPower = var5 / var9 * 0.1;
	zPower = var7 / var9 * 0.1;
}

void Fireball::tick()
{
	Entity::tick();
	onFire = 10;
	if (shakeTime > 0)
	{
		shakeTime--;
	}

	if (inGround)
	{
		int_t var1 = level.getTile(xTile, yTile, zTile);
		if (var1 == lastTile)
		{
			life++;
			if (life == 1200)
			{
				remove();
			}

			return;
		}

		inGround = false;
		xd = xd * (random.nextFloat() * 0.2f);
		yd = yd * (random.nextFloat() * 0.2f);
		zd = zd * (random.nextFloat() * 0.2f);
		life = 0;
		flightTime = 0;
	}
	else
	{
		flightTime++;
	}

	Vec3 *var15 = Vec3::newTemp(x, y, z);
	Vec3 *var2 = Vec3::newTemp(x + xd, y + yd, z + zd);
	HitResult var3 = level.clip(*var15, *var2);
	if (var3.type != HitResult::Type::NONE)
	{
		var2 = Vec3::newTemp(var3.pos->x, var3.pos->y, var3.pos->z);
	}

	Entity *var4 = nullptr;
	AABB *expanded = bb.expand(xd, yd, zd)->grow(1.0, 1.0, 1.0);
	std::vector<std::shared_ptr<Entity>> var5 = level.getEntities(this, *expanded);
	double var6 = 0.0;

	for (size_t var8 = 0; var8 < var5.size(); var8++)
	{
		Entity *var9 = var5[var8].get();
		if (var9->isPickable() && (var9 != owner || flightTime >= 25))
		{
			float var10 = 0.3f;
			AABB var11 = *var9->bb.grow(var10, var10, var10);
			HitResult var12 = var11.clip(*var15, *var2);
			if (var12.type != HitResult::Type::NONE)
			{
				double var13 = var15->distanceTo(*var12.pos);
				if (var13 < var6 || var6 == 0.0)
				{
					var4 = var9;
					var6 = var13;
				}
			}
		}
	}

	if (var4 != nullptr)
	{
		var3 = HitResult(var4);
	}

	if (var3.type != HitResult::Type::NONE)
	{
		if (var3.entity != nullptr && var3.entity->hurt(owner, 0))
		{
		}

		level.explode(nullptr, x, y, z, 1.0f, true);
		remove();
	}

	x = x + xd;
	y = y + yd;
	z = z + zd;
	float var18 = Mth::sqrt(xd * xd + zd * zd);
	yRot = (float)(atan2(xd, zd) * 180.0 / M_PI);
	xRot = (float)(atan2(yd, var18) * 180.0 / M_PI);

	while (xRot - xRotO < -180.0f)
	{
		xRotO -= 360.0f;
	}

	while (xRot - xRotO >= 180.0f)
	{
		xRotO += 360.0f;
	}

	while (yRot - yRotO < -180.0f)
	{
		yRotO -= 360.0f;
	}

	while (yRot - yRotO >= 180.0f)
	{
		yRotO += 360.0f;
	}

	xRot = xRotO + (xRot - xRotO) * 0.2f;
	yRot = yRotO + (yRot - yRotO) * 0.2f;
	float var19 = 0.95f;
	if (isInWater())
	{
		for (int_t var20 = 0; var20 < 4; var20++)
		{
			float var21 = 0.25f;
			level.addParticle(u"bubble", x - xd * var21, y - yd * var21, z - zd * var21, xd, yd, zd);
		}

		var19 = 0.8f;
	}

	xd = xd + xPower;
	yd = yd + yPower;
	zd = zd + zPower;
	xd *= var19;
	yd *= var19;
	zd *= var19;
	level.addParticle(u"smoke", x, y + 0.5, z, 0.0, 0.0, 0.0);
	setPos(x, y, z);
}

void Fireball::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putShort(u"xTile", (short_t)xTile);
	tag.putShort(u"yTile", (short_t)yTile);
	tag.putShort(u"zTile", (short_t)zTile);
	tag.putByte(u"inTile", (byte_t)lastTile);
	tag.putByte(u"shake", (byte_t)shakeTime);
	tag.putByte(u"inGround", (byte_t)(inGround ? 1 : 0));
}

void Fireball::readAdditionalSaveData(CompoundTag &tag)
{
	xTile = tag.getShort(u"xTile");
	yTile = tag.getShort(u"yTile");
	zTile = tag.getShort(u"zTile");
	lastTile = tag.getByte(u"inTile") & 255;
	shakeTime = tag.getByte(u"shake") & 255;
	inGround = tag.getByte(u"inGround") == 1;
}

bool Fireball::isPickable()
{
	return true;
}

float Fireball::getPickRadius()
{
	return 1.0f;
}

bool Fireball::hurt(Entity *var1, int_t var2)
{
	markHurt();
	if (var1 != nullptr)
	{
		Vec3 *var3 = var1->getLookAngle();
		if (var3 != nullptr)
		{
			xd = var3->x;
			yd = var3->y;
			zd = var3->z;
			xPower = xd * 0.1;
			yPower = yd * 0.1;
			zPower = zd * 0.1;
		}

		return true;
	}
	else
	{
		return false;
	}
}

float Fireball::getShadowHeightOffs()
{
	return 0.0f;
}
