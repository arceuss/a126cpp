#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: WorkbenchTile.java - ID 58, texture 59, Material.wood
// Alpha 1.2.6: BlockWorkbench - ID 58, texture 59
class WorkbenchTile : public Tile
{
public:
	WorkbenchTile(int_t id);
	
	// Beta: WorkbenchTile.getTexture() - different textures for different faces (WorkbenchTile.java:14-22)
	virtual int_t getTexture(Facing face, int_t data) override;
	
	// Beta: WorkbenchTile.use() - opens crafting interface (WorkbenchTile.java:25-32)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;
};
