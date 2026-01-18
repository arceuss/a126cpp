#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: PumpkinTile.java - ID 86 (pumpkin) and ID 91 (litPumpkin)
// Alpha: BlockPumpkin (Block.java:108-109)
// PumpkinTile uses Material.vegetable, has orientation-based textures
class PumpkinTile : public Tile
{
private:
	bool lit;  // Beta: PumpkinTile.lit - whether this is a lit pumpkin (ID 91)

public:
	PumpkinTile(int_t id, int_t texture, bool lit);
	
	// Beta: PumpkinTile.getTexture() - orientation-based textures (PumpkinTile.java:19-40, 43-51)
	virtual int_t getTexture(Facing face, int_t data) override;
	virtual int_t getTexture(Facing face) override;
	
	// Beta: PumpkinTile.onPlace() - called when block is placed (PumpkinTile.java:54-56)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PumpkinTile.mayPlace() - requires solid block below (PumpkinTile.java:59-62)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PumpkinTile.setPlacedBy() - sets orientation based on player rotation (PumpkinTile.java:65-68)
	virtual void setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob) override;
};
