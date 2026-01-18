#include "world/level/tile/FenceTile.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"
#include <vector>

// Beta: FenceTile(int var1, int var2) (FenceTile.java:9-11)
FenceTile::FenceTile(int_t id, int_t tex) : Tile(id, tex, Material::wood)
{
	// Beta: FenceTile uses Material.wood (FenceTile.java:10)
}

void FenceTile::addAABBs(Level &level, int_t x, int_t y, int_t z, AABB &bb, std::vector<AABB *> &aabbList)
{
	// Beta: FenceTile.addAABBs() - adds fence AABB (1.5 blocks high) (FenceTile.java:13-15)
	// Beta: var6.add(AABB.newTemp(var2, var3, var4, var2 + 1, var3 + 1.5, var4 + 1))
	AABB *fenceAABB = getAABB(level, x, y, z);
	aabbList.push_back(fenceAABB);  // Beta: Java directly adds without intersection check
}

AABB *FenceTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FenceTile.getAABB() - returns fence AABB (1.5 blocks high) (FenceTile.java:13-15)
	// Beta: AABB.newTemp(var2, var3, var4, var2 + 1, var3 + 1.5, var4 + 1)
	return AABB::newTemp(x, y, z, x + 1, y + 1.5, z + 1);
}

bool FenceTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FenceTile.mayPlace() - checks if can be placed on solid block (FenceTile.java:18-24)
	// Beta: if (var1.getTile(var2, var3 - 1, var4) == this.id) return false
	if (level.getTile(x, y - 1, z) == this->id)
	{
		return false;  // Beta: return false (FenceTile.java:21)
	}
	else
	{
		// Beta: return !var1.getMaterial(var2, var3 - 1, var4).isSolid() ? false : super.mayPlace(var1, var2, var3, var4)
		if (!level.getMaterial(x, y - 1, z).isSolid())
		{
			return false;  // Beta: return false (FenceTile.java:23)
		}
		return Tile::mayPlace(level, x, y, z);  // Beta: return super.mayPlace(var1, var2, var3, var4) (FenceTile.java:23)
	}
}

bool FenceTile::isSolidRender()
{
	// Beta: FenceTile.isSolidRender() returns false (FenceTile.java:31-33)
	return false;  // Beta: return false (FenceTile.java:32)
}

bool FenceTile::isCubeShaped()
{
	// Beta: FenceTile.isCubeShaped() returns false (FenceTile.java:36-38)
	return false;  // Beta: return false (FenceTile.java:37)
}

Tile::Shape FenceTile::getRenderShape()
{
	// Beta: FenceTile.getRenderShape() returns 11 (SHAPE_FENCE) (FenceTile.java:41-43)
	return SHAPE_FENCE;  // Beta: return 11 (FenceTile.java:42)
}
