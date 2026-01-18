#include "world/entity/projectile/FishingHook.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/item/ItemFishingRod.h"
#include "world/entity/item/EntityItem.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "util/Mth.h"
#include "util/Memory.h"
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

// Beta: FishingHook(Level var1) (FishingHook.java:39-42)
FishingHook::FishingHook(Level &level) : Entity(level)
{
	setSize(0.25f, 0.25f);
}

// Beta: FishingHook(Level var1, double var2, double var4, double var6) (FishingHook.java:55-58)
FishingHook::FishingHook(Level &level, double x, double y, double z) : Entity(level)
{
	setSize(0.25f, 0.25f);
	setPos(x, y, z);
}

// Beta: FishingHook(Level var1, Player var2) (FishingHook.java:60-76)
FishingHook::FishingHook(Level &level, Player &owner) : Entity(level)
{
	this->owner = &owner;
	this->owner->fishing = this;  // Beta: this.owner.fishing = this (FishingHook.java:63)
	setSize(0.25f, 0.25f);  // Beta: this.setSize(0.25F, 0.25F) (FishingHook.java:64)
	moveTo(owner.x, owner.y + 1.62f - owner.heightOffset, owner.z, owner.yRot, owner.xRot);  // Beta: this.moveTo(...) (FishingHook.java:65)
	x = x - Mth::cos(yRot / 180.0f * (float)M_PI) * 0.16f;  // Beta: this.x = this.x - Mth.cos(...) (FishingHook.java:66)
	y -= 0.1f;  // Beta: this.y -= 0.1F (FishingHook.java:67)
	z = z - Mth::sin(yRot / 180.0f * (float)M_PI) * 0.16f;  // Beta: this.z = this.z - Mth.sin(...) (FishingHook.java:68)
	setPos(x, y, z);  // Beta: this.setPos(...) (FishingHook.java:69)
	heightOffset = 0.0f;  // Beta: this.heightOffset = 0.0F (FishingHook.java:70)
	float var3 = 0.4f;  // Beta: float var3 = 0.4F (FishingHook.java:71)
	xd = -Mth::sin(yRot / 180.0f * (float)M_PI) * Mth::cos(xRot / 180.0f * (float)M_PI) * var3;  // Beta: this.xd = ... (FishingHook.java:72)
	zd = Mth::cos(yRot / 180.0f * (float)M_PI) * Mth::cos(xRot / 180.0f * (float)M_PI) * var3;  // Beta: this.zd = ... (FishingHook.java:73)
	yd = -Mth::sin(xRot / 180.0f * (float)M_PI) * var3;  // Beta: this.yd = ... (FishingHook.java:74)
	shoot(xd, yd, zd, 1.5f, 1.0f);  // Beta: this.shoot(...) (FishingHook.java:75)
}

void FishingHook::defineSynchedData()
{
	// Beta: defineSynchedData() is empty (FishingHook.java:44-46)
}

// Beta: shoot() - sets velocity and rotation (FishingHook.java:78-96)
void FishingHook::shoot(double var1, double var3, double var5, float var7, float var8)
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

// Beta: lerpTo() - interpolation for multiplayer (FishingHook.java:98-109)
void FishingHook::lerpTo(double var1, double var3, double var5, float var7, float var8, int steps)
{
	lx = var1;
	ly = var3;
	lz = var5;
	lyr = var7;
	lxr = var8;
	lSteps = steps;
	xd = lxd;
	yd = lyd;
	zd = lzd;
}

// Beta: lerpMotion() - motion interpolation (FishingHook.java:111-116)
void FishingHook::lerpMotion(double var1, double var3, double var5)
{
	lxd = xd = var1;
	lyd = yd = var3;
	lzd = zd = var5;
}

// Beta: tick() - main update loop (FishingHook.java:118-310)
void FishingHook::tick()
{
	Entity::tick();
	
	// Beta: Handle interpolation for multiplayer (FishingHook.java:121-139)
	if (lSteps > 0)
	{
		double var22 = x + (lx - x) / lSteps;
		double var24 = y + (ly - y) / lSteps;
		double var25 = z + (lz - z) / lSteps;
		double var7 = lyr - yRot;

		while (var7 < -180.0)
		{
			var7 += 360.0;
		}

		while (var7 >= 180.0)
		{
			var7 -= 360.0;
		}

		yRot = (float)(yRot + var7 / lSteps);
		xRot = (float)(xRot + (lxr - xRot) / lSteps);
		lSteps--;
		setPos(var22, var24, var25);
		setRot(yRot, xRot);
		
		// In multiplayer, only interpolate - don't run physics (server handles it)
		// When lSteps reaches 0, we wait for next server update via lerpTo
		if (level.isOnline)
		{
			return;
		}
	}
	else
	{
		// Beta: In multiplayer, when lSteps == 0, don't run physics (server handles it)
		// Client only interpolates based on server updates
		if (level.isOnline)
		{
			return;
		}
		
		// Beta: Validate player and fishing rod in single-player (FishingHook.java:141-147)
		if (!level.isOnline)
		{
			ItemStack *var1 = owner->getSelectedItem();
			if (owner->removed || !owner->isAlive() || var1 == nullptr || var1->getItem() != Items::fishingRod || distanceToSqr(*owner) > 1024.0)
			{
				remove();
				owner->fishing = nullptr;
				return;
			}

			// Beta: Track hooked entity (FishingHook.java:149-158)
			if (hookedIn != nullptr)
			{
				if (!hookedIn->removed)
				{
					x = hookedIn->x;
					y = hookedIn->bb.y0 + hookedIn->bbHeight * 0.8f;
					z = hookedIn->z;
					return;
				}

				hookedIn = nullptr;
			}
		}

		// Beta: Update shake time (FishingHook.java:161-163)
		if (shakeTime > 0)
		{
			shakeTime--;
		}

		// Beta: Handle in-ground state (FishingHook.java:165-182)
		if (inGround)
		{
			int_t var19 = level.getTile(xTile, yTile, zTile);
			if (var19 == lastTile)
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

		// Beta: Collision detection (FishingHook.java:186-227)
		Vec3 *var20 = Vec3::newTemp(x, y, z);
		Vec3 *var2 = Vec3::newTemp(x + xd, y + yd, z + zd);
		HitResult var3 = level.clip(*var20, *var2);
		var20 = Vec3::newTemp(x, y, z);
		var2 = Vec3::newTemp(x + xd, y + yd, z + zd);
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
				HitResult var12 = var11.clip(*var20, *var2);
				if (var12.type != HitResult::Type::NONE)
				{
					double var13 = var20->distanceTo(*var12.pos);
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
				if (var3.entity->hurt(owner, 0))
				{
					hookedIn = var3.entity.get();
				}
			}
			else
			{
				inGround = true;
			}
		}

		// Beta: Movement and water physics (FishingHook.java:229-308)
		if (!inGround)
		{
			move(xd, yd, zd);
			float var26 = Mth::sqrt(xd * xd + zd * zd);
			yRot = (float)(atan2(xd, zd) * 180.0 / M_PI);
			xRot = (float)(atan2(yd, var26) * 180.0 / M_PI);

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
			float var27 = 0.92f;
			if (onGround || horizontalCollision)
			{
				var27 = 0.5f;
			}

			// Beta: Check water coverage (FishingHook.java:258-268)
			byte_t var28 = 5;
			double var29 = 0.0;

			for (int_t var30 = 0; var30 < var28; var30++)
			{
				double var14 = bb.y0 + (bb.y1 - bb.y0) * (var30 + 0) / var28 - 0.125 + 0.125;
				double var16 = bb.y0 + (bb.y1 - bb.y0) * (var30 + 1) / var28 - 0.125 + 0.125;
				AABB *var18 = AABB::newTemp(bb.x0, var14, bb.z0, bb.x1, var16, bb.z1);
				// Beta: containsLiquid check - check for water material (FishingHook.java:265)
				// TODO: Add Level::containsLiquid(AABB, Material) helper if needed
				// For now, use simple water material check
				if (level.containsAnyLiquid(*var18))
				{
					// Additional check to ensure it's water specifically
					int_t x0 = Mth::floor(var18->x0);
					int_t x1 = Mth::floor(var18->x1 + 1.0);
					int_t y0 = Mth::floor(var18->y0);
					int_t y1 = Mth::floor(var18->y1 + 1.0);
					int_t z0 = Mth::floor(var18->z0);
					int_t z1 = Mth::floor(var18->z1 + 1.0);
					
					bool inWater = false;
					for (int_t xx = x0; xx < x1 && !inWater; xx++)
					{
						for (int_t yy = y0; yy < y1 && !inWater; yy++)
						{
							for (int_t zz = z0; zz < z1 && !inWater; zz++)
							{
								const Material &mat = level.getMaterial(xx, yy, zz);  // Beta: Use getMaterial() like FarmTile
								if (mat == Material::water)
								{
									int_t data = level.getData(xx, yy, zz);
									double waterLevel = yy + 1;
									if (data < 8)
									{
										waterLevel = yy + 1 - data / 8.0;
									}
									if (waterLevel >= var18->y0)
									{
										inWater = true;
									}
								}
							}
						}
					}
					
					if (inWater)
					{
						var29 += 1.0 / var28;
					}
				}
			}

			// Beta: Handle fish nibbling (FishingHook.java:270-290)
			if (var29 > 0.0)
			{
				if (nibble > 0)
				{
					nibble--;
				}
				else if (random.nextInt(500) == 0)
				{
					nibble = random.nextInt(30) + 10;
					yd -= 0.2f;
					level.playSound(this, u"random.splash", 0.25f, 1.0f + (random.nextFloat() - random.nextFloat()) * 0.4f);
					float var31 = Mth::floor(bb.y0);

					for (int_t var33 = 0; var33 < static_cast<int_t>(1.0f + bbWidth * 20.0f); var33++)
					{
						float var15 = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
						float var36 = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
						level.addParticle(u"bubble", x + var15, var31 + 1.0f, z + var36, xd, yd - random.nextFloat() * 0.2f, zd);
					}

					for (int_t var34 = 0; var34 < static_cast<int_t>(1.0f + bbWidth * 20.0f); var34++)
					{
						float var35 = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
						float var37 = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
						level.addParticle(u"splash", x + var35, var31 + 1.0f, z + var37, xd, yd, zd);
					}
				}
			}

			if (nibble > 0)
			{
				yd = yd - random.nextFloat() * random.nextFloat() * random.nextFloat() * 0.2;
			}

			double var32 = var29 * 2.0 - 1.0;
			yd += 0.04f * var32;
			if (var29 > 0.0)
			{
				var27 = (float)(var27 * 0.9);
				yd *= 0.8;
			}

			xd *= var27;
			yd *= var27;
			zd *= var27;
			setPos(x, y, z);
		}
	}
}

// Beta: retrieve() - retrieves the hook and returns damage amount (FishingHook.java:337-370)
int_t FishingHook::retrieve()
{
	byte_t var1 = 0;
	if (hookedIn != nullptr)
	{
		double var2 = owner->x - x;
		double var4 = owner->y - y;
		double var6 = owner->z - z;
		double var8 = Mth::sqrt(var2 * var2 + var4 * var4 + var6 * var6);
		double var10 = 0.1;
		hookedIn->xd += var2 * var10;
		hookedIn->yd = hookedIn->yd + (var4 * var10 + Mth::sqrt(var8) * 0.08);
		hookedIn->zd += var6 * var10;
		var1 = 3;
	}
	else if (nibble > 0)
	{
		// Beta: Spawn fish item (FishingHook.java:350-359)
		int_t fishId = 93;  // Beta 1.2: Item.fish_raw.id = 93 (fallback)
		if (Items::fishRaw != nullptr)
		{
			fishId = static_cast<int_t>(Items::fishRaw->getShiftedIndex());
		}
		ItemStack stack(fishId, 1);
		auto var13 = Util::make_shared<EntityItem>(level, x, y, z, stack);
		double var3 = owner->x - x;
		double var5 = owner->y - y;
		double var7 = owner->z - z;
		double var9 = Mth::sqrt(var3 * var3 + var5 * var5 + var7 * var7);
		double var11 = 0.1;
		var13->xd = var3 * var11;
		var13->yd = var5 * var11 + Mth::sqrt(var9) * 0.08;
		var13->zd = var7 * var11;
		level.addEntity(var13);
		var1 = 1;
	}

	if (inGround)
	{
		var1 = 2;
	}

	remove();
	owner->fishing = nullptr;
	return var1;
}

// Beta: addAdditionalSaveData() - save hook state (FishingHook.java:312-320)
void FishingHook::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putShort(u"xTile", (short_t)xTile);
	tag.putShort(u"yTile", (short_t)yTile);
	tag.putShort(u"zTile", (short_t)zTile);
	tag.putByte(u"inTile", (byte_t)lastTile);
	tag.putByte(u"shake", (byte_t)shakeTime);
	tag.putByte(u"inGround", (byte_t)(inGround ? 1 : 0));
}

// Beta: readAdditionalSaveData() - load hook state (FishingHook.java:322-330)
void FishingHook::readAdditionalSaveData(CompoundTag &tag)
{
	xTile = tag.getShort(u"xTile");
	yTile = tag.getShort(u"yTile");
	zTile = tag.getShort(u"zTile");
	lastTile = tag.getByte(u"inTile") & 255;
	shakeTime = tag.getByte(u"shake") & 255;
	inGround = tag.getByte(u"inGround") == 1;
}

// Beta: getShadowHeightOffs() - returns 0.0F (FishingHook.java:332-335)
float FishingHook::getShadowHeightOffs()
{
	return 0.0f;
}
