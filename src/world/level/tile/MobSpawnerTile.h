#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2 Tile.mobSpawner (ID 52, texture 65)
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/tile/MobSpawnerTile.java
class MobSpawnerTile : public Tile
{
public:
	MobSpawnerTile(int_t id, int_t texture);
	
	// Beta: MobSpawnerTile.newTileEntity() returns MobSpawnerTileEntity
	virtual std::shared_ptr<TileEntity> newTileEntity();
	
	// Beta: MobSpawnerTile.getResource() returns 0 (drops nothing)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: MobSpawnerTile.isSolidRender() returns false
	virtual bool isSolidRender() override;
};
