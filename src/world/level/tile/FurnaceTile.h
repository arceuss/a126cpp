#pragma once

#include "world/level/tile/Tile.h"

class Mob;  // Forward declaration

// Beta 1.2: FurnaceTile.java - ID 61 (unlit), ID 62 (lit)
// Alpha 1.2.6: Furnace blocks exist but may differ in behavior
class FurnaceTile : public Tile
{
private:
	bool lit;  // Beta: private final boolean lit (FurnaceTile.java:14)

public:
	FurnaceTile(int_t id, bool lit);
	
	// Beta: FurnaceTile.newTileEntity() returns FurnaceTileEntity
	virtual std::shared_ptr<TileEntity> newTileEntity() override;
	
	// Beta: FurnaceTile.getResource() - always returns furnace (unlit) ID (FurnaceTile.java:22-25)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: FurnaceTile.onPlace() - calls recalcLockDir (FurnaceTile.java:27-31)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: EntityTile.onRemove() - removes TileEntity (EntityTile.java:24-28)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: FurnaceTile.getTexture() - different textures based on face and lit state (FurnaceTile.java:58-72)
	virtual int_t getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Beta: FurnaceTile.getTexture() - overload for Facing with data (for item rendering)
	virtual int_t getTexture(Facing face, int_t data) override;
	
	// Beta: FurnaceTile.getTexture() - overload for Facing only (FurnaceTile.java:99-108)
	virtual int_t getTexture(Facing face) override;
	
	// Beta: FurnaceTile.animateTick() - particles when lit (FurnaceTile.java:74-97)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: FurnaceTile.use() - opens furnace GUI (FurnaceTile.java:110-119)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;
	
	// Beta: FurnaceTile.setPlacedBy() - sets orientation based on player rotation (FurnaceTile.java:139-157)
	// Beta: @Override in Java - overrides Tile.setPlacedBy()
	virtual void setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob) override;
	
	// Beta: FurnaceTile.setLit() - static method to toggle between lit/unlit (FurnaceTile.java:121-132)
	static void setLit(bool lit, Level &level, int_t x, int_t y, int_t z);
	
private:
	// Beta: FurnaceTile.recalcLockDir() - calculates orientation based on adjacent solid blocks (FurnaceTile.java:33-56)
	void recalcLockDir(Level &level, int_t x, int_t y, int_t z);
};
