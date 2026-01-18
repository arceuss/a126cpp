#include "world/level/ChunkPos.h"

#include "world/entity/Entity.h"

double ChunkPos::distanceToSqr(const Entity &entity)
{
	double var2 = x * 16 + 8;
	double var4 = z * 16 + 8;
	double var6 = var2 - entity.x;
	double var8 = var4 - entity.z;
	return var6 * var6 + var8 * var8;
}