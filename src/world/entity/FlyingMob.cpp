#include "world/entity/FlyingMob.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "util/Mth.h"

FlyingMob::FlyingMob(Level &level) : Mob(level)
{
}

void FlyingMob::causeFallDamage(float distance)
{
	// FlyingMob doesn't take fall damage
}

void FlyingMob::travel(float x, float z)
{
	if (isInWater())
	{
		moveRelative(x, z, 0.02f);
		move(xd, yd, zd);
		xd *= 0.8f;
		yd *= 0.8f;
		zd *= 0.8f;
	}
	else if (isInLava())
	{
		moveRelative(x, z, 0.02f);
		move(xd, yd, zd);
		xd *= 0.5;
		yd *= 0.5;
		zd *= 0.5;
	}
	else
	{
		float var3 = 0.91f;
		if (onGround)
		{
			var3 = 0.54600006f;
			int_t var4 = level.getTile(Mth::floor(this->x), Mth::floor(bb.y0) - 1, Mth::floor(this->z));
			if (var4 > 0)
			{
				var3 = Tile::tiles[var4]->friction * 0.91f;
			}
		}

		float var10 = 0.16277136f / (var3 * var3 * var3);
		moveRelative(x, z, onGround ? 0.1f * var10 : 0.02f);
		var3 = 0.91f;
		if (onGround)
		{
			var3 = 0.54600006f;
			int_t var5 = level.getTile(Mth::floor(this->x), Mth::floor(bb.y0) - 1, Mth::floor(this->z));
			if (var5 > 0)
			{
				var3 = Tile::tiles[var5]->friction * 0.91f;
			}
		}

		move(xd, yd, zd);
		xd *= var3;
		yd *= var3;
		zd *= var3;
	}

	walkAnimSpeedO = walkAnimSpeed;
	double var9 = this->x - xo;
	double var11 = this->z - zo;
	float var7 = Mth::sqrt(var9 * var9 + var11 * var11) * 4.0f;
	if (var7 > 1.0f)
	{
		var7 = 1.0f;
	}

	walkAnimSpeed = walkAnimSpeed + (var7 - walkAnimSpeed) * 0.4f;
	walkAnimPos = walkAnimPos + walkAnimSpeed;
}

bool FlyingMob::onLadder()
{
	return false;
}
