#pragma once

#include "world/level/tile/Tile.h"
#include "world/entity/Entity.h"
#include "world/entity/Mob.h"
#include "world/entity/player/Player.h"
#include <vector>
#include <memory>

// Beta 1.2: PressurePlateTile.java - ID 70 (stone), ID 72 (wood), Material.stone
// Alpha 1.2.6: BlockPressurePlate - ID 70 (pressurePlateStone), ID 72 (pressurePlatePlanks)
class PressurePlateTile : public Tile
{
public:
	enum Sensitivity
	{
		everything,
		mobs,
		players
	};

private:
	Sensitivity sensitivity;

	// Beta: PressurePlateTile.checkPressed() - checks if entities are pressing (PressurePlateTile.java:88-128)
	void checkPressed(Level &level, int_t x, int_t y, int_t z);

public:
	PressurePlateTile(int_t id, int_t tex, Sensitivity sensitivity);
	
	// Beta: PressurePlateTile.getTickDelay() - returns 20 (PressurePlateTile.java:24-27)
	virtual int_t getTickDelay() override;
	
	// Beta: PressurePlateTile.getAABB() - returns null (no collision) (PressurePlateTile.java:29-32)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PressurePlateTile.isSolidRender() - returns false (PressurePlateTile.java:34-37)
	virtual bool isSolidRender() override;
	
	// Beta: PressurePlateTile.blocksLight() - returns false (PressurePlateTile.java:39-41)
	bool blocksLight();
	
	// Beta: PressurePlateTile.isCubeShaped() - returns false (PressurePlateTile.java:43-46)
	virtual bool isCubeShaped() override;
	
	// Beta: PressurePlateTile.mayPlace() - checks if can be placed on solid block (PressurePlateTile.java:48-51)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PressurePlateTile.onPlace() - empty (PressurePlateTile.java:53-55)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PressurePlateTile.neighborChanged() - breaks if support removed (PressurePlateTile.java:57-68)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: PressurePlateTile.tick() - checks if pressed (PressurePlateTile.java:70-77)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: PressurePlateTile.entityInside() - checks if pressed (PressurePlateTile.java:79-86)
	virtual void entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity) override;
	
	// Beta: PressurePlateTile.onRemove() - updates neighbors if pressed (PressurePlateTile.java:130-139)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PressurePlateTile.updateShape() - updates shape based on pressed state (PressurePlateTile.java:141-150)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;
	
	// Beta: PressurePlateTile.getSignal() - returns true if pressed (PressurePlateTile.java:152-155)
	virtual bool getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing) override;
	
	// Beta: PressurePlateTile.getDirectSignal() - returns direct signal (PressurePlateTile.java:157-160)
	virtual bool getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing) override;
	
	// Beta: PressurePlateTile.isSignalSource() - returns true (PressurePlateTile.java:162-165)
	virtual bool isSignalSource() override;
	
	// Beta: PressurePlateTile.updateDefaultShape() - updates default shape (PressurePlateTile.java:167-173)
	virtual void updateDefaultShape() override;
};
