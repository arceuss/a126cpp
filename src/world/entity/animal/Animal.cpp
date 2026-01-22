#include "world/entity/animal/Animal.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "nbt/CompoundTag.h"
#include "util/Mth.h"
#include <iostream>

Animal::Animal(Level &level) : PathfinderMob(level)
{
}

float Animal::getWalkTargetValue(int_t x, int_t y, int_t z)
{
	return level.getTile(x, y - 1, z) == Tile::grass.id ? 10.0f : level.getBrightness(x, y, z) - 0.5f;
}

void Animal::addAdditionalSaveData(CompoundTag &tag)
{
	PathfinderMob::addAdditionalSaveData(tag);
}

void Animal::readAdditionalSaveData(CompoundTag &tag)
{
	PathfinderMob::readAdditionalSaveData(tag);
}

bool Animal::canSpawn()
{
	int_t var1 = Mth::floor(this->x);
	int_t var2 = Mth::floor(bb.y0);
	int_t var3 = Mth::floor(this->z);
	int_t tileBelow = level.getTile(var1, var2 - 1, var3);
	int_t brightness = level.getRawBrightness(var1, var2, var3);
	bool grassCheck = tileBelow == Tile::grass.id;
	bool brightnessCheck = brightness > 8;
	bool pathfinderCheck = PathfinderMob::canSpawn();
	return grassCheck && brightnessCheck && pathfinderCheck;
}

int_t Animal::getAmbientSoundInterval()
{
	return 120;
}
