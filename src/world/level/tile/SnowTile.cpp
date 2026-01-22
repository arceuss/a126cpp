#include "world/level/tile/SnowTile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/level/LightLayer.h"
#include "world/phys/AABB.h"
#include "java/Random.h"

// Beta: Tile.topSnow = new TopSnowTile(78, 66).setDestroyTime(0.1F).setSoundType(SOUND_CLOTH)
// Beta: TopSnowTile uses Material.topSnow (DecorationMaterial), not Material.snow
SnowTile::SnowTile(int_t id, int_t texture) : TransparentTile(id, texture, Material::topSnow, false)
{
	setDestroyTime(0.1f);
	// Step sound is handled by Material - no need to set explicitly
	setTicking(true);
	
	// Beta: TopSnowTile.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.125F, 1.0F) - 0.125F = 2.0F / 16.0F
	setShape(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
	
	// Beta: TopSnowTile.blocksLight() returns false - handled by DecorationMaterial
	lightBlock[id] = 0;
	translucent[id] = true;
}

AABB *SnowTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha: BlockSnow.getCollisionBoundingBoxFromPool() returns null
	return nullptr;  // Null AABB (no collision)
}

bool SnowTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: TopSnowTile.mayPlace() (TopSnowTile.java:40-43)
	int_t belowTile = level.getTile(x, y - 1, z);
	if (belowTile == 0)
		return false;
	Tile *belowTileObj = Tile::tiles[belowTile];
	if (belowTileObj == nullptr || !belowTileObj->isSolidRender())
		return false;
	return level.getMaterial(x, y - 1, z).blocksMotion();
}

bool SnowTile::checkCanSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: TopSnowTile.checkCanSurvive() (TopSnowTile.java:50-58)
	if (!mayPlace(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
		return false;
	}
	return true;
}

void SnowTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: TopSnowTile.neighborChanged() calls checkCanSurvive() (TopSnowTile.java:46-48)
	checkCanSurvive(level, x, y, z);
}

int_t SnowTile::getResource(int_t data, Random &random)
{
	// Beta: TopSnowTile.getResource() returns Item.snowBall.id (TopSnowTile.java:74-76)
	// TODO: Return Item.snowBall ID once items are implemented
	return 0;  // Placeholder - will be Item::snowBall->id
}

int_t SnowTile::getResourceCount(Random &random)
{
	// Alpha: BlockSnow.quantityDropped() returns 0
	return 0;
}

void SnowTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: TopSnowTile.tick() - melts if light > 11 (TopSnowTile.java:84-89)
	if (level.getBrightness(LightLayer::Block, x, y, z) > 11)
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
	}
}

bool SnowTile::shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: TopSnowTile.shouldRenderFace() (TopSnowTile.java:92-99)
	const Material &neighborMat = level.getMaterial(x, y, z);
	if (face == Facing::UP)  // Face 1 (UP)
		return true;
	if (neighborMat == Material::topSnow)  // Same material - don't render
		return false;
	return TransparentTile::shouldRenderFace(level, x, y, z, face);
}

void SnowTile::playerDestroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	// Beta: TopSnowTile.playerDestroy() - manually spawns ItemEntity (TopSnowTile.java:61-71)
	// TODO: Implement ItemEntity spawning once Item.snowBall is available
	// int_t snowBallId = Item::snowBall.id;
	// float offset = 0.7f;
	// double dx = level.random.nextFloat() * offset + (1.0f - offset) * 0.5;
	// double dy = level.random.nextFloat() * offset + (1.0f - offset) * 0.5;
	// double dz = level.random.nextFloat() * offset + (1.0f - offset) * 0.5;
	// ItemEntity *item = new ItemEntity(level, x + dx, y + dy, z + dz, ItemStack(snowBallId, 1, 0));
	// item->throwTime = 10;
	// level.addEntity(item);
	level.setTile(x, y, z, 0);
}
