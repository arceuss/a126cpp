#pragma once

#include "world/level/tile/Tile.h"
#include <array>

// Beta 1.2: FireTile.java - ID 51, texture 31, Material.fire
// Alpha 1.2.6: BlockFire - ID 51, texture 31
class FireTile : public Tile
{
public:
	// Beta: FireTile constants (FireTile.java:10-18)
	static constexpr int_t FLAME_INSTANT = 60;
	static constexpr int_t FLAME_EASY = 30;
	static constexpr int_t FLAME_MEDIUM = 15;
	static constexpr int_t FLAME_HARD = 5;
	static constexpr int_t BURN_INSTANT = 100;
	static constexpr int_t BURN_EASY = 60;
	static constexpr int_t BURN_MEDIUM = 20;
	static constexpr int_t BURN_HARD = 5;
	static constexpr int_t BURN_NEVER = 0;

private:
	std::array<int_t, 256> flameOdds;  // Beta: flameOdds[256] (FireTile.java:19)
	std::array<int_t, 256> burnOdds;   // Beta: burnOdds[256] (FireTile.java:20)

public:
	FireTile(int_t id, int_t tex);
	
	// Beta: FireTile.getAABB() returns null (no collision) (FireTile.java:39-41)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: FireTile.blocksLight() returns false (FireTile.java:43-45)
	bool blocksLight() { return false; }
	
	// Beta: FireTile.isSolidRender() returns false (FireTile.java:48-50)
	virtual bool isSolidRender() override;
	
	// Beta: FireTile.isCubeShaped() returns false (FireTile.java:53-55)
	virtual bool isCubeShaped() override;
	
	// Beta: FireTile.getRenderShape() returns 3 (FireTile.java:58-60)
	virtual Shape getRenderShape() override;
	
	// Beta: FireTile.getResourceCount() returns 0 (FireTile.java:63-65)
	virtual int_t getResourceCount(Random &random) override;
	
	// Beta: FireTile.getTickDelay() returns 10 (FireTile.java:68-70)
	virtual int_t getTickDelay() override;
	
	// Beta: FireTile.tick() - complex fire spread logic (FireTile.java:73-115)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: FireTile.mayPick() returns false (FireTile.java:162-164)
	virtual bool mayPick() override;
	
	// Beta: FireTile.canBurn() - checks if block can burn (FireTile.java:166-168)
	bool canBurn(LevelSource &level, int_t x, int_t y, int_t z);
	
	// Beta: FireTile.canBlockCatchFire() - static method for rendering (RenderBlocks.java uses Tile.fire.canBlockCatchFire())
	static bool canBlockCatchFire(LevelSource &level, int_t x, int_t y, int_t z);
	
	// Beta: FireTile.getFlammability() - gets flammability value (FireTile.java:170-173)
	int_t getFlammability(Level &level, int_t x, int_t y, int_t z, int_t current);
	
	// Beta: FireTile.mayPlace() - checks if fire can be placed (FireTile.java:176-178)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: FireTile.neighborChanged() - checks if fire can survive (FireTile.java:181-185)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: FireTile.onPlace() - validates fire placement (FireTile.java:188-196)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: FireTile.isFlammable() - checks if block ID is flammable (FireTile.java:198-200)
	bool isFlammable(int_t tileId);
	
	// Beta: FireTile.ignite() - tries to ignite nearby blocks (FireTile.java:202-231)
	void ignite(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: FireTile.animateTick() - adds smoke particles and sounds (FireTile.java:234-292)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
private:
	// Beta: FireTile.setFlammable() - sets flammability for a block (FireTile.java:33-36)
	void setFlammable(int_t tileId, int_t flameOdds, int_t burnOdds);
	
	// Beta: FireTile.checkBurn() - checks if block should burn (FireTile.java:117-131)
	void checkBurn(Level &level, int_t x, int_t y, int_t z, int_t chance, Random &random);
	
	// Beta: FireTile.isValidFireLocation() - checks if fire can exist here (FireTile.java:133-145)
	bool isValidFireLocation(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: FireTile.getFireOdds() - calculates fire spread odds (FireTile.java:147-159)
	int_t getFireOdds(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: FireTile.tryIgnite() - tries to place fire at location (FireTile.java:294-304)
	bool tryIgnite(Level &level, int_t x, int_t y, int_t z);
};
