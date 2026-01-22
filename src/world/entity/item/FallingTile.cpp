#include "world/entity/item/FallingTile.h"

#include "world/level/Level.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"

FallingTile::FallingTile(Level &level) : Entity(level)
{
}

FallingTile::FallingTile(Level &level, double x, double y, double z, int_t tile) : FallingTile(level)
{
	this->tile = tile;
	blocksBuilding = true;
	setSize(0.98f, 0.98f);
	heightOffset = bbHeight / 2.0f;
	setPos(x, y, z);
	xd = 0.0;
	yd = 0.0;
	zd = 0.0;
	makeStepSound = false;
	xo = x;
	yo = y;
	zo = z;
}

void FallingTile::defineSynchedData()
{
}

bool FallingTile::isPickable()
{
	return !removed;
}

void FallingTile::tick()
{
	if (tile == 0)
	{
		remove();
	}
	else
	{
		// newb12: Remove block immediately at start of tick to prevent flicker
		// Java: Block is removed in tick() before movement (FallingTile.java:56-58)
		// We remove it at the start to prevent both block and entity rendering on first frame
		int_t var1 = Mth::floor(x);
		int_t var2 = Mth::floor(y);
		int_t var3 = Mth::floor(z);
		if (level.getTile(var1, var2, var3) == tile)
		{
			level.setTile(var1, var2, var3, 0);
		}
		
		xo = x;
		yo = y;
		zo = z;
		time++;
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
			remove();
			if ((!level.mayPlace(tile, var1, var2, var3, true) || !level.setTile(var1, var2, var3, tile)) && !level.isOnline)
			{
				spawnAtLocation(tile, 1);
			}
		}
		else if (time > 100 && !level.isOnline)
		{
			spawnAtLocation(tile, 1);
			remove();
		}
	}
}

void FallingTile::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putByte(u"Tile", (byte_t)tile);
}

void FallingTile::readAdditionalSaveData(CompoundTag &tag)
{
	tile = tag.getByte(u"Tile") & 255;
}

float FallingTile::getShadowHeightOffs()
{
	return 0.0f;
}
