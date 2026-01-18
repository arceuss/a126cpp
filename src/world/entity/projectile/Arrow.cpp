#include "world/entity/projectile/Arrow.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/InventoryPlayer.h"
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

Arrow::Arrow(Level &level) : Entity(level)
{
	setSize(0.5f, 0.5f);
}

Arrow::Arrow(Level &level, double x, double y, double z) : Entity(level)
{
	setSize(0.5f, 0.5f);
	setPos(x, y, z);
	heightOffset = 0.0f;
}

Arrow::Arrow(Level &level, Mob &owner) : Entity(level)
{
	this->owner = &owner;
	setSize(0.5f, 0.5f);
	moveTo(owner.x, owner.y + owner.getHeadHeight(), owner.z, owner.yRot, owner.xRot);
	x = x - Mth::cos(yRot / 180.0f * (float)M_PI) * 0.16f;
	y -= 0.1f;
	z = z - Mth::sin(yRot / 180.0f * (float)M_PI) * 0.16f;
	setPos(x, y, z);
	heightOffset = 0.0f;
	xd = -Mth::sin(yRot / 180.0f * (float)M_PI) * Mth::cos(xRot / 180.0f * (float)M_PI);
	zd = Mth::cos(yRot / 180.0f * (float)M_PI) * Mth::cos(xRot / 180.0f * (float)M_PI);
	yd = -Mth::sin(xRot / 180.0f * (float)M_PI);
	shoot(xd, yd, zd, 1.5f, 1.0f);
}

void Arrow::defineSynchedData()
{
}

void Arrow::shoot(double var1, double var3, double var5, float var7, float var8)
{
	float var9 = Mth::sqrt(var1 * var1 + var3 * var3 + var5 * var5);
	var1 /= var9;
	var3 /= var9;
	var5 /= var9;
	var1 += nextGaussian(random) * 0.0075f * var8;
	var3 += nextGaussian(random) * 0.0075f * var8;
	var5 += nextGaussian(random) * 0.0075f * var8;
	var1 *= var7;
	var3 *= var7;
	var5 *= var7;
	xd = var1;
	yd = var3;
	zd = var5;
	float var10 = Mth::sqrt(var1 * var1 + var5 * var5);
	yRotO = yRot = (float)(atan2(var1, var5) * 180.0 / M_PI);
	xRotO = xRot = (float)(atan2(var3, var10) * 180.0 / M_PI);
	life = 0;
}

void Arrow::lerpMotion(double var1, double var3, double var5)
{
	Entity::lerpMotion(var1, var3, var5);
}

void Arrow::tick()
{
	Entity::tick();
	if (xRotO == 0.0f && yRotO == 0.0f)
	{
		float var1 = Mth::sqrt(xd * xd + zd * zd);
		yRotO = yRot = (float)(atan2(xd, zd) * 180.0 / M_PI);
		xRotO = xRot = (float)(atan2(yd, var1) * 180.0 / M_PI);
	}

	if (shakeTime > 0)
	{
		shakeTime--;
	}

	if (inGround)
	{
		int_t var15 = level.getTile(xTile, yTile, zTile);
		if (var15 == lastTile)
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

	Vec3 *var16 = Vec3::newTemp(x, y, z);
	Vec3 *var2 = Vec3::newTemp(x + xd, y + yd, z + zd);
	HitResult var3 = level.clip(*var16, *var2);
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
		if (var9->isPickable() && (var9 != owner || flightTime >= 5))
		{
			float var10 = 0.3f;
			AABB var11 = *var9->bb.grow(var10, var10, var10);
			HitResult var12 = var11.clip(*var16, *var2);
			if (var12.type != HitResult::Type::NONE)
			{
				double var13 = var16->distanceTo(*var12.pos);
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
		if (var3.entity != nullptr)
		{
			if (var3.entity->hurt(owner, 4))
			{
				// Alpha 1.2.6: Only play sound in single-player (EntityArrow.java:150-151)
				// Beta 1.2: No isOnline check in Arrow.java, but we need it to prevent duplicate sounds in multiplayer
				// The server handles arrow hits and sends packets, so client shouldn't play sounds
				if (!level.isOnline)
				{
					level.playSound(this, u"random.drr", 1.0f, 1.2f / (random.nextFloat() * 0.2f + 0.9f));
				}
				remove();
			}
			else
			{
				xd *= -0.1f;
				yd *= -0.1f;
				zd *= -0.1f;
				yRot += 180.0f;
				yRotO += 180.0f;
				flightTime = 0;
			}
		}
		else
		{
			xTile = var3.x;
			yTile = var3.y;
			zTile = var3.z;
			lastTile = level.getTile(xTile, yTile, zTile);
			xd = (float)(var3.pos->x - x);
			yd = (float)(var3.pos->y - y);
			zd = (float)(var3.pos->z - z);
			float var19 = Mth::sqrt(xd * xd + yd * yd + zd * zd);
			x = x - xd / var19 * 0.05f;
			y = y - yd / var19 * 0.05f;
			z = z - zd / var19 * 0.05f;
			// Alpha 1.2.6: Only play sound in single-player (similar check needed for block hits)
			// Beta 1.2: No isOnline check in Arrow.java, but we need it to prevent duplicate sounds in multiplayer
			if (!level.isOnline)
			{
				level.playSound(this, u"random.drr", 1.0f, 1.2f / (random.nextFloat() * 0.2f + 0.9f));
			}
			inGround = true;
			shakeTime = 7;
		}
	}

	x = x + xd;
	y = y + yd;
	z = z + zd;
	float var20 = Mth::sqrt(xd * xd + zd * zd);
	yRot = (float)(atan2(xd, zd) * 180.0 / M_PI);
	xRot = (float)(atan2(yd, var20) * 180.0 / M_PI);

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
	float var21 = 0.99f;
	float var22 = 0.03f;
	if (isInWater())
	{
		for (int_t var23 = 0; var23 < 4; var23++)
		{
			float var24 = 0.25f;
			level.addParticle(u"bubble", x - xd * var24, y - yd * var24, z - zd * var24, xd, yd, zd);
		}

		var21 = 0.8f;
	}

	xd *= var21;
	yd *= var21;
	zd *= var21;
	yd -= var22;
	setPos(x, y, z);
}

void Arrow::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putShort(u"xTile", (short_t)xTile);
	tag.putShort(u"yTile", (short_t)yTile);
	tag.putShort(u"zTile", (short_t)zTile);
	tag.putByte(u"inTile", (byte_t)lastTile);
	tag.putByte(u"shake", (byte_t)shakeTime);
	tag.putByte(u"inGround", (byte_t)(inGround ? 1 : 0));
}

void Arrow::readAdditionalSaveData(CompoundTag &tag)
{
	xTile = tag.getShort(u"xTile");
	yTile = tag.getShort(u"yTile");
	zTile = tag.getShort(u"zTile");
	lastTile = tag.getByte(u"inTile") & 255;
	shakeTime = tag.getByte(u"shake") & 255;
	inGround = tag.getByte(u"inGround") == 1;
}

void Arrow::playerTouch(Player &player)
{
	if (!level.isOnline)
	{
		if (inGround && owner == &player && shakeTime <= 0)
		{
		if (Items::arrow != nullptr)
		{
			ItemStack stack(static_cast<int_t>(Items::arrow->getShiftedIndex()), static_cast<int_t>(1));
				if (player.inventory.add(stack))
				{
					level.playSound(this, u"random.pop", 0.2f, ((random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
					player.take(*this, 1);
					remove();
				}
			}
		}
	}
}

float Arrow::getShadowHeightOffs()
{
	return 0.0f;
}
