#pragma once

#include "world/level/tile/FluidTile.h"

// Beta 1.2: LiquidTileDynamic.java - ported 1:1
class FluidFlowingTile : public FluidTile
{
private:
	int_t maxCount = 0;  // Beta: int maxCount = 0
	bool result[4];      // Beta: boolean[] result = new boolean[4]
	int_t dist[4];       // Beta: int[] dist = new int[4]

public:
	FluidFlowingTile(int_t id, const LiquidMaterial &material);

	// Beta: LiquidTileDynamic.tick() - main flow logic (LiquidTileDynamic.java:23-115)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;

	// Beta: LiquidTileDynamic.onPlace() schedules update (LiquidTileDynamic.java:257-263)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;

private:
	// Beta: LiquidTileDynamic.setStatic() converts to static block (LiquidTileDynamic.java:16-21)
	void setStatic(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTileDynamic.trySpreadTo() spreads to position (LiquidTileDynamic.java:117-130)
	void trySpreadTo(Level &level, int_t x, int_t y, int_t z, int_t fluidLevel);
	
	// Beta: LiquidTileDynamic.getSlopeDistance() calculates distance to source (LiquidTileDynamic.java:132-172)
	int_t getSlopeDistance(Level &level, int_t x, int_t y, int_t z, int_t distance, int_t fromDir);
	
	// Beta: LiquidTileDynamic.getSpread() calculates spread directions (LiquidTileDynamic.java:174-217)
	bool *getSpread(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTileDynamic.isWaterBlocking() checks if block blocks water (LiquidTileDynamic.java:219-229)
	bool isWaterBlocking(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTileDynamic.getHighest() gets highest neighbor level (LiquidTileDynamic.java:231-246)
	int_t getHighest(Level &level, int_t x, int_t y, int_t z, int_t currentMin);
	
	// Beta: LiquidTileDynamic.canSpreadTo() checks if can spread (LiquidTileDynamic.java:248-255)
	bool canSpreadTo(Level &level, int_t x, int_t y, int_t z);
};
