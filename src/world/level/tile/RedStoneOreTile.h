#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: RedStoneOreTile.java - ID 73 (unlit), ID 74 (lit), texture 51, Material.stone
// Alpha 1.2.6: BlockRedstoneOre - ID 73 (oreRedstone), ID 74 (oreRedstoneGlowing)
class RedStoneOreTile : public Tile
{
private:
	bool lit;

	// Beta: RedStoneOreTile.interact() - triggers glow (RedStoneOreTile.java:45-50)
	void interact(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: RedStoneOreTile.poofParticles() - adds reddust particles (RedStoneOreTile.java:76-112)
	void poofParticles(Level &level, int_t x, int_t y, int_t z);

public:
	RedStoneOreTile(int_t id, int_t tex, bool lit);
	
	// Beta: RedStoneOreTile.getTickDelay() - returns 30 (RedStoneOreTile.java:22-25)
	virtual int_t getTickDelay() override;
	
	// Beta: RedStoneOreTile.attack() - triggers glow (RedStoneOreTile.java:27-31)
	virtual void attack(Level &level, int_t x, int_t y, int_t z, Player &player) override;
	
	// Beta: RedStoneOreTile.stepOn() - triggers glow (RedStoneOreTile.java:33-37)
	virtual void stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
	
	// Beta: RedStoneOreTile.use() - triggers glow (RedStoneOreTile.java:39-43)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;
	
	// Beta: RedStoneOreTile.tick() - turns off lit version after 30 ticks (RedStoneOreTile.java:52-57)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: RedStoneOreTile.getResource() - returns redstone item (RedStoneOreTile.java:59-62)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: RedStoneOreTile.getResourceCount() - returns 4-5 redstone (RedStoneOreTile.java:64-67)
	virtual int_t getResourceCount(Random &random) override;
	
	// Beta: RedStoneOreTile.animateTick() - adds particles if lit (RedStoneOreTile.java:69-74)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
};
