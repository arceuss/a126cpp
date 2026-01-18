#include "world/entity/monster/PathfinderMob.h"
#include "world/level/Level.h"
#include "world/level/pathfinder/Path.h"
#include "world/entity/player/Player.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

// newb12: PathfinderMob
// Reference: newb12/net/minecraft/world/entity/PathfinderMob.java
PathfinderMob::PathfinderMob(Level &level) : Mob(level)
{
}

void PathfinderMob::updateAi()
{
	// newb12: PathfinderMob.updateAi() - handles pathfinding and movement (PathfinderMob.java:19-140)
	holdGround = false;
	float var1 = 16.0F;
	
	// newb12: Attack target logic (PathfinderMob.java:22-34)
	if (attackTarget == nullptr)
	{
		attackTarget = findAttackTarget();
		if (attackTarget != nullptr)
		{
			path = level.findPath(*this, *attackTarget, var1);
		}
	}
	else if (!attackTarget->isAlive())
	{
		attackTarget = nullptr;
	}
	else
	{
		// newb12: Calculate distance from attackTarget to this (PathfinderMob.java:30)
		float var2 = attackTarget->distanceTo(*this);
		if (canSee(*attackTarget))
		{
			checkHurtTarget(*attackTarget, var2);
		}
	}
	
	// newb12: Pathfinding logic (PathfinderMob.java:36-64)
	if (holdGround || attackTarget == nullptr || (path != nullptr && random.nextInt(20) != 0))
	{
		if ((path == nullptr && random.nextInt(80) == 0) || random.nextInt(80) == 0)
		{
			bool var21 = false;
			int_t var3 = -1;
			int_t var4 = -1;
			int_t var5 = -1;
			float var6 = -99999.0F;
			
			// newb12: Random wander target search (PathfinderMob.java:44-56)
			for (int_t var7 = 0; var7 < 10; var7++)
			{
				int_t var8 = Mth::floor(x + random.nextInt(13) - 6.0);
				int_t var9 = Mth::floor(y + random.nextInt(7) - 3.0);
				int_t var10 = Mth::floor(z + random.nextInt(13) - 6.0);
				float var11 = getWalkTargetValue(var8, var9, var10);
				if (var11 > var6)
				{
					var6 = var11;
					var3 = var8;
					var4 = var9;
					var5 = var10;
					var21 = true;
				}
			}
			
			// newb12: Create path to wander target (PathfinderMob.java:58-60)
			if (var21)
			{
				path = level.findPath(*this, var3, var4, var5, 10.0F);
			}
		}
	}
	else
	{
		// newb12: Path to attack target (PathfinderMob.java:62-64)
		if (attackTarget != nullptr)
			path = level.findPath(*this, *attackTarget, var1);
	}
	
	// newb12: Movement along path (PathfinderMob.java:66-135)
	int_t var22 = Mth::floor(bb.y0);
	bool var23 = isInWater();
	bool var24 = isInLava();
	xRot = 0.0F;
	
	if (path != nullptr && random.nextInt(100) != 0)
	{
		// newb12: Get current path node (PathfinderMob.java:71)
		Vec3 *var25 = path->current(*this);
		double var26 = bbWidth * 2.0F;
		
		// newb12: Advance path if close to current node (PathfinderMob.java:74-82)
		while (var25 != nullptr && var25->distanceToSqr(x, var25->y, z) < var26 * var26)
		{
			path->next();
			if (path->isDone())
			{
				var25 = nullptr;
				path.reset();
			}
			else
			{
				var25 = path->current(*this);
			}
		}
		
		// newb12: Movement toward path node (PathfinderMob.java:84-123)
		jumping = false;
		if (var25 != nullptr)
		{
			double var27 = var25->x - x;
			double var28 = var25->z - z;
			double var12 = var25->y - var22;
			float var14 = (float)(std::atan2(var28, var27) * 180.0 / M_PI) - 90.0F;
			float var15 = var14 - yRot;
			yya = runSpeed;
			
			while (var15 < -180.0F)
				var15 += 360.0F;
			
			while (var15 >= 180.0F)
				var15 -= 360.0F;
			
			if (var15 > 30.0F)
				var15 = 30.0F;
			
			if (var15 < -30.0F)
				var15 = -30.0F;
			
			yRot += var15;
			
			// newb12: Hold ground logic (PathfinderMob.java:110-118)
			if (holdGround && attackTarget != nullptr)
			{
				double var16 = attackTarget->x - x;
				double var18 = attackTarget->z - z;
				float var20 = yRot;
				yRot = (float)(std::atan2(var18, var16) * 180.0 / M_PI) - 90.0F;
				var15 = (var20 - yRot + 90.0F) * (float)M_PI / 180.0F;
				xxa = -Mth::sin(var15) * yya * 1.0F;
				yya = Mth::cos(var15) * yya * 1.0F;
			}
			
			// newb12: Jump if path goes up (PathfinderMob.java:120-122)
			if (var12 > 0.0)
			{
				jumping = true;
			}
		}
		
		// newb12: Look at attack target (PathfinderMob.java:125-127)
		if (attackTarget != nullptr)
		{
			lookAt(*attackTarget, 30.0F);
		}
		
		// newb12: Jump on collision (PathfinderMob.java:129-131)
		if (horizontalCollision)
		{
			jumping = true;
		}
		
		// newb12: Jump in water/lava (PathfinderMob.java:133-135)
		if (random.nextFloat() < 0.8F && (var23 || var24))
		{
			jumping = true;
		}
	}
	else
	{
		// newb12: No path - fall back to base Mob AI (PathfinderMob.java:136-139)
		Mob::updateAi();
		path.reset();
	}
}

void PathfinderMob::checkHurtTarget(Entity &target, float distance)
{
	// newb12: PathfinderMob.checkHurtTarget() - default empty (PathfinderMob.java:142-143)
}

float PathfinderMob::getWalkTargetValue(int_t x, int_t y, int_t z)
{
	// newb12: PathfinderMob.getWalkTargetValue() - default returns 0.0F (PathfinderMob.java:145-147)
	return 0.0F;
}

std::shared_ptr<Entity> PathfinderMob::findAttackTarget()
{
	// newb12: PathfinderMob.findAttackTarget() - default returns null (PathfinderMob.java:149-151)
	return nullptr;
}

bool PathfinderMob::canSpawn()
{
	// newb12: PathfinderMob.canSpawn() - checks getWalkTargetValue >= 0.0F (PathfinderMob.java:153-159)
	int_t var1 = Mth::floor(x);
	int_t var2 = Mth::floor(bb.y0);
	int_t var3 = Mth::floor(z);
	return Mob::canSpawn() && getWalkTargetValue(var1, var2, var3) >= 0.0F;
}
