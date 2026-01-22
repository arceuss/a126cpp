#include "world/level/tile/ReedTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/item/Items.h"
#include "java/Random.h"

// Beta: ReedTile.java - uses Material.plant (DecorationMaterial)
ReedTile::ReedTile(int_t id, int_t tex) : TransparentTile(id, tex, Material::plant, false)
{
	// Beta: ReedTile.setShape() - 0.375F = 6/16 (ReedTile.java:13-14)
	float size = 0.375f;  // Beta: 0.375F
	setShape(0.5f - size, 0.0f, 0.5f - size, 0.5f + size, 1.0f, 0.5f + size);
	
	setTicking(true);
	setDestroyTime(0.0f);  // Beta: hardness 0.0F (Tile.java:229)
}

bool ReedTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ReedTile.mayPlace() (ReedTile.java:40-53)
	int_t belowId = level.getTile(x, y - 1, z);
	if (belowId == id)
		return true;
	if (belowId != Tile::grass.id && belowId != Tile::dirt.id)
		return false;
	// Check for water adjacent (any of 4 cardinal directions below)
	if (level.getMaterial(x - 1, y - 1, z) == Material::water)
		return true;
	if (level.getMaterial(x + 1, y - 1, z) == Material::water)
		return true;
	if (level.getMaterial(x, y - 1, z - 1) == Material::water)
		return true;
	return level.getMaterial(x, y - 1, z + 1) == Material::water;
}

bool ReedTile::canSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ReedTile.canSurvive() delegates to mayPlace() (ReedTile.java:68-70)
	return mayPlace(level, x, y, z);
}

void ReedTile::checkAlive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ReedTile.checkAlive() (ReedTile.java:60-65)
	if (!canSurvive(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
	}
}

int_t ReedTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockReed.idDropped() returns Item.reed.shiftedIndex (BlockReed.java:58-59)
	// Beta: ReedTile.getResource() returns Item.reeds.id (ReedTile.java:78-80)
	return Items::reed != nullptr ? Items::reed->getShiftedIndex() : 0;
}

void ReedTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: ReedTile.tick() - grows upward if space above and height < 3 (ReedTile.java:19-37)
	if (level.isEmptyTile(x, y + 1, z))
	{
		// Count height (how many reeds below)
		int_t height = 1;
		while (level.getTile(x, y - height, z) == id)
			height++;
		
		// Beta: Max height is 3 (ReedTile.java:27)
		if (height < 3)
		{
			// Beta: Uses metadata 0-15 for growth, when reaches 15, grows block above (ReedTile.java:28-34)
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

void ReedTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: ReedTile.neighborChanged() calls checkAlive() (ReedTile.java:56-58)
	checkAlive(level, x, y, z);
}

Tile::Shape ReedTile::getRenderShape()
{
	// Beta: ReedTile.getRenderShape() returns 1 (ReedTile.java:97-99)
	return SHAPE_CROSS_TEXTURE;  // Shape 1 = SHAPE_CROSS_TEXTURE
}

bool ReedTile::isCubeShaped()
{
	// Beta: ReedTile.isCubeShaped() returns false (ReedTile.java:92-94)
	return false;
}

bool ReedTile::isSolidRender()
{
	// Beta: ReedTile.isSolidRender() returns false (ReedTile.java:87-89)
	return false;
}

AABB *ReedTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ReedTile.getAABB() returns null (ReedTile.java:73-75)
	return nullptr;
}
