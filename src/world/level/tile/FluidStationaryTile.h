#pragma once

#include "world/level/tile/FluidTile.h"

// Beta 1.2: LiquidTileStatic.java - ported 1:1
class FluidStationaryTile : public FluidTile
{
public:
	FluidStationaryTile(int_t id, const LiquidMaterial &material);

	// Beta: LiquidTileStatic.neighborChanged() converts to dynamic (LiquidTileStatic.java:16-22)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;

	// Beta: LiquidTileStatic.tick() - lava can spread fire (LiquidTileStatic.java:33-58)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;

private:
	// Beta: LiquidTileStatic.setDynamic() converts to dynamic block (LiquidTileStatic.java:24-31)
	void setDynamic(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTileStatic.isFlammable() checks if block is flammable (LiquidTileStatic.java:60-62)
	bool isFlammable(Level &level, int_t x, int_t y, int_t z);
};
