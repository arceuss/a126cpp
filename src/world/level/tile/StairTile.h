#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: StairTile.java - ID 53 (stairs_wood), ID 67 (stairs_stone)
// Alpha 1.2.6: BlockStair - ID 53 (stairCompactPlanks), ID 67 (stairCompactCobblestone)
class StairTile : public Tile
{
private:
	Tile *base;  // Beta: private Tile base (StairTile.java:15)

public:
	StairTile(int_t id, Tile &baseTile);
	
	// Beta: StairTile overrides multiple methods to delegate to base tile
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	virtual bool isSolidRender() override;
	virtual bool isCubeShaped() override;
	virtual Shape getRenderShape() override;
	virtual void addAABBs(Level &level, int_t x, int_t y, int_t z, AABB &bb, std::vector<AABB *> &aabbList) override;
	
	// Beta: Delegating methods to base tile
	virtual void addLights(Level &level, int_t x, int_t y, int_t z) override;
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	virtual void attack(Level &level, int_t x, int_t y, int_t z, Player &player) override;
	virtual void destroy(Level &level, int_t x, int_t y, int_t z, int_t data) override;
	virtual float getBrightness(LevelSource &level, int_t x, int_t y, int_t z) override;
	virtual int_t getRenderLayer() override;
	virtual int_t getResource(int_t data, Random &random) override;
	virtual int_t getResourceCount(Random &random) override;
	virtual int_t getTexture(Facing face, int_t data) override;
	virtual int_t getTexture(Facing face) override;
	virtual int_t getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	virtual int_t getTickDelay() override;
	virtual AABB *getTileAABB(Level &level, int_t x, int_t y, int_t z) override;
	virtual bool mayPick() override;
	virtual bool mayPick(int_t data, bool canPickLiquid) override;
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	virtual void prepareRender(Level &level, int_t x, int_t y, int_t z) override;
	virtual void stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;
	
	// Beta: StairTile.setPlacedBy() - sets orientation based on player rotation (StairTile.java:220-237)
	virtual void setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob) override;
};
