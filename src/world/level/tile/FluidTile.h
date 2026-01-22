#pragma once

#include "world/level/tile/Tile.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/phys/Vec3.h"

// Beta 1.2: LiquidTile.java - ported 1:1
class FluidTile : public Tile
{
protected:
	const LiquidMaterial &fluidMaterial;

public:
	FluidTile(int_t id, const LiquidMaterial &material);
	
	// Beta: LiquidTile.getHeight() - static helper (LiquidTile.java:20-26)
	static float getHeight(int_t metadata);
	
	// Beta: LiquidTile.getTexture() - side textures (LiquidTile.java:28-31)
	virtual int_t getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: LiquidTile.isCubeShaped() returns false (LiquidTile.java:50-53)
	virtual bool isCubeShaped() override;
	
	// Beta: LiquidTile.isSolidRender() returns false (LiquidTile.java:55-58)
	virtual bool isSolidRender() const;
	
	// Beta: LiquidTile.mayPick() - only with bucket when level 0 (LiquidTile.java:60-63)
	virtual bool mayPick(int_t data, bool withTool) override;
	
	// Beta: LiquidTile.shouldRenderFace() - hides faces between same fluid (LiquidTile.java:65-75)
	virtual bool shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: LiquidTile.getAABB() returns null (no collision) (LiquidTile.java:77-80)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LiquidTile.getRenderShape() returns 4 (LiquidTile.java:82-85)
	virtual Shape getRenderShape() override;
	
	// Beta: LiquidTile.getResource() returns 0 (LiquidTile.java:87-90)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: LiquidTile.getResourceCount() returns 0 (LiquidTile.java:92-95)
	virtual int_t getResourceCount(Random &random) override;
	
	// Beta: LiquidTile.getTickDelay() - water=5, lava=30 (LiquidTile.java:185-192)
	virtual int_t getTickDelay() override;
	
	// Beta: LiquidTile.getBrightness() - max of current and above (LiquidTile.java:194-199)
	virtual float getBrightness(LevelSource &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LiquidTile.getRenderLayer() - 1 for water, 0 for lava (LiquidTile.java:206-209)
	virtual int_t getRenderLayer() override;
	
	// Beta: LiquidTile.onPlace() calls updateLiquid (LiquidTile.java:244-247)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: LiquidTile.neighborChanged() calls updateLiquid (LiquidTile.java:249-252)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: LiquidTile.handleEntityInside() - applies flow vector (LiquidTile.java:177-183)
	virtual void handleEntityInside(Level &level, int_t x, int_t y, int_t z, Entity *entity, Vec3 &motion);
	
	// Beta: LiquidTile.animateTick() - water sounds, lava particles (LiquidTile.java:211-229)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: LiquidTile.getSlopeAngle() - static helper for flow direction (LiquidTile.java:230-242)
	static double getSlopeAngle(LevelSource &level, int_t x, int_t y, int_t z, const Material &material);

protected:
	// Beta: LiquidTile.getDepth() - gets fluid level at position (LiquidTile.java:33-35)
	int_t getDepth(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTile.getRenderedDepth() - gets rendered depth (normalized) (LiquidTile.java:37-48)
	int_t getRenderedDepth(LevelSource &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTile.getFlow() - calculates flow vector (LiquidTile.java:97-175)
	Vec3 *getFlow(LevelSource &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTile.updateLiquid() - checks for water+lava hardening (LiquidTile.java:254-290)
	void updateLiquid(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: LiquidTile.fizz() - plays fizz sound and particles (LiquidTile.java:292-298)
	void fizz(Level &level, int_t x, int_t y, int_t z);
};
