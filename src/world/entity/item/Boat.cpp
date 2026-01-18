#include "world/entity/item/Boat.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "world/entity/player/Player.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

Boat::Boat(Level &level) : Entity(level)
{
	blocksBuilding = true;
	setSize(1.5f, 0.6f);
	heightOffset = bbHeight / 2.0f;
	makeStepSound = false;
}

Boat::Boat(Level &level, double x, double y, double z) : Boat(level)
{
	setPos(x, y + heightOffset, z);
	xd = 0.0;
	yd = 0.0;
	zd = 0.0;
	xo = x;
	yo = y;
	zo = z;
}

void Boat::defineSynchedData()
{
}

AABB *Boat::getCollideAgainstBox(Entity &entity)
{
	return &entity.bb;
}

AABB *Boat::getCollideBox()
{
	return &bb;
}

bool Boat::isPushable()
{
	return true;
}

double Boat::getRideHeight()
{
	return bbHeight * 0.0 - 0.3f;
}

bool Boat::hurt(Entity *var1, int_t var2)
{
	if (!level.isOnline && !removed)
	{
		hurtDir = -hurtDir;
		hurtTime = 10;
		damage += var2 * 10;
		markHurt();
		if (damage > 40)
		{
			for (int_t var3 = 0; var3 < 3; var3++)
			{
				spawnAtLocation(5, 1, 0.0f);  // Tile.wood.id = 5
			}

			for (int_t var4 = 0; var4 < 2; var4++)
			{
				spawnAtLocation(Items::stick->getShiftedIndex(), 1, 0.0f);
			}

			remove();
		}

		return true;
	}
	else
	{
		return true;
	}
}

void Boat::animateHurt()
{
	hurtDir = -hurtDir;
	hurtTime = 10;
	damage = damage + damage * 10;
}

bool Boat::isPickable()
{
	return !removed;
}

void Boat::lerpTo(double var1, double var3, double var5, float var7, float var8, int_t var9)
{
	lx = var1;
	ly = var3;
	lz = var5;
	lyr = var7;
	lxr = var8;
	lSteps = var9 + 4;
	xd = lxd;
	yd = lyd;
	zd = lzd;
}

void Boat::lerpMotion(double var1, double var3, double var5)
{
	lxd = xd = var1;
	lyd = yd = var3;
	lzd = zd = var5;
}

void Boat::tick()
{
	Entity::tick();
	if (hurtTime > 0)
	{
		hurtTime--;
	}

	if (damage > 0)
	{
		damage--;
	}

	xo = x;
	yo = y;
	zo = z;
	byte_t var1 = 5;
	double var2 = 0.0;

	for (int_t var4 = 0; var4 < var1; var4++)
	{
		double var5 = bb.y0 + (bb.y1 - bb.y0) * (var4 + 0) / var1 - 0.125;
		double var7 = bb.y0 + (bb.y1 - bb.y0) * (var4 + 1) / var1 - 0.125;
		AABB *var9 = AABB::newTemp(bb.x0, var5, bb.z0, bb.x1, var7, bb.z1);
		if (level.containsAnyLiquid(*var9))
		{
			var2 += 1.0 / var1;
		}
	}

	if (level.isOnline)
	{
		if (lSteps > 0)
		{
			double var24 = x + (lx - x) / lSteps;
			double var26 = y + (ly - y) / lSteps;
			double var28 = z + (lz - z) / lSteps;
			double var33 = lyr - yRot;

			while (var33 < -180.0)
			{
				var33 += 360.0;
			}

			while (var33 >= 180.0)
			{
				var33 -= 360.0;
			}

			yRot = (float)(yRot + var33 / lSteps);
			xRot = (float)(xRot + (lxr - xRot) / lSteps);
			lSteps--;
			setPos(var24, var26, var28);
			setRot(yRot, xRot);
		}
		else
		{
			double var25 = x + xd;
			double var27 = y + yd;
			double var29 = z + zd;
			setPos(var25, var27, var29);
			if (onGround)
			{
				xd *= 0.5;
				yd *= 0.5;
				zd *= 0.5;
			}

			xd *= 0.99f;
			yd *= 0.95f;
			zd *= 0.99f;
		}
		
		// Position rider after boat moves (even in multiplayer)
		// The rider's rideTick() will also call positionRider(), but we need to
		// ensure the rider is positioned immediately after the boat moves
		if (rider != nullptr)
		{
			positionRider();
		}
	}
	else
	{
		double var23 = var2 * 2.0 - 1.0;
		yd += 0.04f * var23;
		if (rider != nullptr)
		{
			// Alpha 1.2.6: EntityBoat.onUpdate() - uses rider's motion for boat movement
			// Java: this.motionX += this.riddenByEntity.motionX * 0.2D;
			//       this.motionZ += this.riddenByEntity.motionZ * 0.2D;
			// When riding, the player's velocity is zeroed in rideTick() before tick(),
			// but tick() processes input and sets velocity. The boat's tick happens
			// before the rider's rideTick(), so it reads velocity from the previous tick.
			// The player should process input and set velocity, which the boat will read.
			// For now, use velocity as Java does - the player's tick() should set it.
			Mob* riderMob = dynamic_cast<Mob*>(rider.get());
			if (riderMob != nullptr)
			{
				// Use rider's input (xxa, yya) converted to velocity for boat control
				// This ensures the boat responds to player input even when velocity is zeroed
				double inputX = static_cast<double>(riderMob->getXxa()) * 0.2;
				double inputZ = static_cast<double>(riderMob->getYya()) * 0.2;
				xd = xd + inputX;
				zd = zd + inputZ;
			}
			else
			{
				// Fallback to velocity for non-Mob riders
				xd = xd + rider->xd * 0.2;
				zd = zd + rider->zd * 0.2;
			}
		}

		double var6 = 0.4;
		if (xd < -var6)
		{
			xd = -var6;
		}

		if (xd > var6)
		{
			xd = var6;
		}

		if (zd < -var6)
		{
			zd = -var6;
		}

		if (zd > var6)
		{
			zd = var6;
		}

		if (onGround)
		{
			xd *= 0.5;
			yd *= 0.5;
			zd *= 0.5;
		}

		move(xd, yd, zd);
		double var8 = Mth::sqrt(xd * xd + zd * zd);
		if (var8 > 0.15)
		{
			double var10 = std::cos(yRot * M_PI / 180.0);
			double var12 = std::sin(yRot * M_PI / 180.0);

			for (int_t var14 = 0; var14 < 1.0 + var8 * 60.0; var14++)
			{
				double var15 = random.nextFloat() * 2.0f - 1.0f;
				double var17 = (random.nextInt(2) * 2 - 1) * 0.7;
				if (random.nextBoolean())
				{
					double var19 = x - var10 * var15 * 0.8 + var12 * var17;
					double var21 = z - var12 * var15 * 0.8 - var10 * var17;
					level.addParticle(u"splash", var19, y - 0.125, var21, xd, yd, zd);
				}
				else
				{
					double var36 = x + var10 + var12 * var15 * 0.7;
					double var38 = z + var12 - var10 * var15 * 0.7;
					level.addParticle(u"splash", var36, y - 0.125, var38, xd, yd, zd);
			}
		}
	}

	// Beta 1.2: Boat.tick() - check if rider is removed (Boat.java:295-297)
	if (rider != nullptr && rider->removed)
	{
		rider = nullptr;
	}

	if (!horizontalCollision || !(var8 > 0.15))
	{
		xd *= 0.99f;
		yd *= 0.95f;
		zd *= 0.99f;
	}
	else if (!level.isOnline)
	{
		remove();

			for (int_t var30 = 0; var30 < 3; var30++)
			{
				spawnAtLocation(5, 1, 0.0f);  // Tile.wood.id = 5
			}

			for (int_t var31 = 0; var31 < 2; var31++)
			{
				spawnAtLocation(Items::stick->getShiftedIndex(), 1, 0.0f);
			}
		}

		xRot = 0.0f;
		double var32 = yRot;
		double var34 = xo - x;
		double var35 = zo - z;
		if (var34 * var34 + var35 * var35 > 0.001)
		{
			var32 = (float)(std::atan2(var35, var34) * 180.0 / M_PI);
		}

		double var16 = var32 - yRot;

		while (var16 >= 180.0)
		{
			var16 -= 360.0;
		}

		while (var16 < -180.0)
		{
			var16 += 360.0;
		}

		if (var16 > 20.0)
		{
			var16 = 20.0;
		}

		if (var16 < -20.0)
		{
			var16 = -20.0;
		}

		yRot = (float)(yRot + var16);
		setRot(yRot, xRot);
		AABB expanded = *bb.grow(0.2f, 0.0, 0.2f);
		std::vector<std::shared_ptr<Entity>> var18 = level.getEntities(this, expanded);
		if (var18.size() > 0)
		{
			for (size_t var37 = 0; var37 < var18.size(); var37++)
			{
				Entity *var20 = var18[var37].get();
				if (var20 != rider.get() && var20->isPushable() && dynamic_cast<Boat*>(var20) != nullptr)
				{
					var20->push(*this);
				}
			}
		}

		if (rider != nullptr && rider->removed)
		{
			rider = nullptr;
		}
	}
}

void Boat::positionRider()
{
	if (rider != nullptr)
	{
		double var1 = std::cos(yRot * M_PI / 180.0) * 0.4;
		double var3 = std::sin(yRot * M_PI / 180.0) * 0.4;
		rider->setPos(x + var1, y + getRideHeight() + rider->getRidingHeight(), z + var3);
	}
}

void Boat::addAdditionalSaveData(CompoundTag &tag)
{
}

void Boat::readAdditionalSaveData(CompoundTag &tag)
{
}

float Boat::getShadowHeightOffs()
{
	return 0.0f;
}

bool Boat::interact(Player &var1)
{
	// Beta 1.2: Boat.interact() - matches newb12/net/minecraft/world/entity/item/Boat.java:331-339 exactly
	// Beta: if (this.rider != null && this.rider instanceof Player && this.rider != var1) { return true; }
	if (rider != nullptr && dynamic_cast<Player*>(rider.get()) != nullptr && rider.get() != &var1)
	{
		return true;
	}
	else
	{
		// Beta: if (!this.level.isOnline) { var1.ride(this); }
		if (!level.isOnline)
		{
			// Find this boat in the level's entities to get a shared_ptr for ride()
			std::shared_ptr<Entity> boatPtr = nullptr;
			for (const auto &e : level.getAllEntities())
			{
				if (e.get() == this)
				{
					boatPtr = e;
					break;
				}
			}
			if (boatPtr)
			{
				var1.ride(boatPtr);
			}
		}
		// Beta: Multiplayer: don't mount locally - wait for Packet39 from server
		// The GameMode::interact() will send Packet7 to server, which processes it and sends Packet39 back

		// Beta: return true;
		return true;
	}
}
