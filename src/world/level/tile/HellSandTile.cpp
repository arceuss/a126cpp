#include "world/level/tile/HellSandTile.h"
#include "world/level/Level.h"
#include "world/entity/Entity.h"
#include "world/phys/AABB.h"

// Beta: HellSandTile.java
HellSandTile::HellSandTile(int_t id, int_t texture) : Tile(id, texture, Material::sand)
{
	// Beta: Properties set in initTiles()
}

AABB *HellSandTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: HellSandTile.getAABB() (HellSandTile.java:14-16)
	// Returns AABB with height reduced by 0.125F (1/8 block)
	float var5 = 0.125F;
	return AABB::newTemp(x, y, z, x + 1, y + 1 - var5, z + 1);
}

void HellSandTile::entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: HellSandTile.entityInside() (HellSandTile.java:20-22)
	// Slows entities by reducing xd and zd to 0.4
	entity.xd *= 0.4;
	entity.zd *= 0.4;
}
