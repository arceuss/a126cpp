#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2 Tile.chest (ID 54, texture 26)
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/tile/ChestTile.java
class ChestTile : public Tile
{
public:
	ChestTile(int_t id);
	
	// Beta: ChestTile.newTileEntity() returns ChestTileEntity
	virtual std::shared_ptr<TileEntity> newTileEntity();
	
	// Beta: EntityTile.onPlace() - creates TileEntity (EntityTile.java:19-22)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: EntityTile.onRemove() - removes TileEntity (EntityTile.java:24-28)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: ChestTile.getTexture() - complex texture logic for chest faces (ChestTile.java:23-105)
	virtual int_t getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: ChestTile.getTexture() - overload for Facing only (ChestTile.java:107-116)
	virtual int_t getTexture(Facing face) override;
	
	// Beta: ChestTile.use() - opens chest interface (ChestTile.java:192-228)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;
};
