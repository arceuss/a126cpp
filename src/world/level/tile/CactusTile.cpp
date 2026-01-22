#include "world/level/tile/CactusTile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/material/Material.h"
#include "java/Random.h"
#include "Facing.h"

// Beta: CactusTile.java
CactusTile::CactusTile(int_t id, int_t tex) : Tile(id, tex, Material::cactus)
{
	setDestroyTime(0.4f);  // Beta: hardness 0.4F (Tile.java:227)
	setTicking(true);
}

bool CactusTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: CactusTile.mayPlace() - checks super.mayPlace() and canSurvive() (CactusTile.java:73-75)
	if (!Tile::mayPlace(level, x, y, z))
		return false;
	return canSurvive(level, x, y, z);
}

bool CactusTile::canSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: CactusTile.canSurvive() (CactusTile.java:86-99)
	// Check adjacent blocks are not solid
	if (level.getMaterial(x - 1, y, z).isSolid())
		return false;
	if (level.getMaterial(x + 1, y, z).isSolid())
		return false;
	if (level.getMaterial(x, y, z - 1).isSolid())
		return false;
	if (level.getMaterial(x, y, z + 1).isSolid())
		return false;
	
	// Check below is sand or cactus
	int_t belowId = level.getTile(x, y - 1, z);
	return belowId == Tile::sand.id || belowId == id;
}

void CactusTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: CactusTile.tick() - grows upward if space above and height < 3 (CactusTile.java:16-34)
	if (level.isEmptyTile(x, y + 1, z))
	{
		// Count height (how many cacti below)
		int_t height = 1;
		while (level.getTile(x, y - height, z) == id)
			height++;
		
		// Beta: Max height is 3 (CactusTile.java:24)
		if (height < 3)
		{
			// Beta: Uses metadata 0-15 for growth, when reaches 15, grows block above (CactusTile.java:25-31)
			int_t data = level.getData(x, y, z);
			if (data == 15)
			{
				level.setTile(x, y + 1, z, id);
				level.setData(x, y, z, 0);
			}
			else
			{
				level.setData(x, y, z, data + 1);
			}
		}
	}
}

void CactusTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: CactusTile.neighborChanged() - checks canSurvive, removes if not (CactusTile.java:78-83)
	if (!canSurvive(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
	}
}

void CactusTile::entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: CactusTile.entityInside() - hurts entities (CactusTile.java:102-104)
	entity.hurt(nullptr, 1);
}

Tile::Shape CactusTile::getRenderShape()
{
	// Beta: CactusTile.getRenderShape() returns 13 (CactusTile.java:68-70)
	return SHAPE_CACTUS;  // Shape 13 = SHAPE_CACTUS
}

bool CactusTile::isCubeShaped()
{
	// Beta: CactusTile.isCubeShaped() returns false (CactusTile.java:58-60)
	return false;
}

bool CactusTile::isSolidRender()
{
	// Beta: CactusTile.isSolidRender() returns false (CactusTile.java:63-64)
	return false;
}

int_t CactusTile::getTexture(Facing face)
{
	// Beta: CactusTile.getTexture() (CactusTile.java:49-54)
	// Face 1 (UP) = tex - 1, Face 0 (DOWN) = tex + 1, others = tex
	if (face == Facing::UP)  // Face 1
		return tex - 1;
	else if (face == Facing::DOWN)  // Face 0
		return tex + 1;
	else
		return tex;
}

AABB *CactusTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: CactusTile.getAABB() - custom collision box (CactusTile.java:37-40)
	float margin = 0.0625f;  // 1/16
	return AABB::newTemp(x + margin, y, z + margin, x + 1 - margin, y + 1 - margin, z + 1 - margin);
}

AABB *CactusTile::getTileAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: CactusTile.getTileAABB() - custom collision box for tile (CactusTile.java:43-46)
	float margin = 0.0625f;  // 1/16
	return AABB::newTemp(x + margin, y, z + margin, x + 1 - margin, y + 1, z + 1 - margin);
}