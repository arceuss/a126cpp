#include "world/level/tile/MobSpawnerTile.h"
#include "world/level/material/Material.h"
#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "world/level/tile/entity/TileEntity.h"
#include "java/Random.h"
#include <memory>

// Beta: MobSpawnerTile(id, 65) - Material.stone, hardness 5.0F
MobSpawnerTile::MobSpawnerTile(int_t id, int_t texture) : Tile(id, texture, Material::stone)
{
	setDestroyTime(5.0f);
	// Beta: MobSpawnerTile extends EntityTile, so isEntityTile should be true
	Tile::isEntityTile[id] = true;
	// Beta: MobSpawnerTile.isSolidRender() returns false
	Tile::solid[id] = false;
	Tile::lightBlock[id] = 0;  // Beta: blocksLight() returns false
}

std::shared_ptr<TileEntity> MobSpawnerTile::newTileEntity()
{
	// Beta: MobSpawnerTile.newTileEntity() returns new MobSpawnerTileEntity()
	return std::make_shared<MobSpawnerTileEntity>();
}

int_t MobSpawnerTile::getResource(int_t data, Random &random)
{
	// Beta: MobSpawnerTile.getResource() returns 0 (drops nothing)
	return 0;
}

bool MobSpawnerTile::isSolidRender()
{
	// Beta: MobSpawnerTile.isSolidRender() returns false
	return false;
}
