#pragma once

#include "world/entity/Entity.h"
#include "world/level/tile/Tile.h"
#include "world/level/TilePos.h"
#include "world/phys/AABB.h"
#include "world/phys/Vec3.h"
#include "java/Random.h"
#include "java/Type.h"
#include "util/Memory.h"
#include <unordered_set>
#include <vector>

class Level;

// Alpha 1.2.6 Explosion
// Reference: newb12/net/minecraft/world/level/Explosion.java
class Explosion
{
public:
	bool fire = false;

private:
	Random random;
	Level &level;

public:
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	Entity *source = nullptr;
	float r = 0.0f;
	std::unordered_set<TilePos> toBlow;

public:
	Explosion(Level &level, Entity *source, double x, double y, double z, float radius);

public:
	void explode();
	void addParticles();
};
