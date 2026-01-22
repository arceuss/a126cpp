#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: ButtonTile.java - ID 77, texture rock.tex
// Alpha 1.2.6: Button blocks exist
class ButtonTile : public Tile
{
public:
	ButtonTile(int_t id, int_t tex);

	// Beta: ButtonTile.getAABB() - returns null (no collision) (ButtonTile.java:17-19)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: ButtonTile.getTickDelay() - returns 20 (ButtonTile.java:22-24)
	virtual int_t getTickDelay() override;

	// Beta: ButtonTile.blocksLight() returns false (ButtonTile.java:26-28)
	bool blocksLight() { return false; }

	// Beta: ButtonTile.isSolidRender() returns false (ButtonTile.java:31-33)
	virtual bool isSolidRender() override;

	// Beta: ButtonTile.isCubeShaped() returns false (ButtonTile.java:36-38)
	virtual bool isCubeShaped() override;

	// Beta: ButtonTile.mayPlace() - checks if can be placed on solid block (ButtonTile.java:41-49)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: ButtonTile.setPlacedOnFace() - sets orientation based on face (ButtonTile.java:52-73)
	virtual void setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face) override;

	// Beta: ButtonTile.onPlace() - sets orientation based on adjacent solid blocks (ButtonTile.java:76-88)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: ButtonTile.neighborChanged() - checks if button can survive (ButtonTile.java:91-116)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;

	// Beta: ButtonTile.updateShape() - sets shape based on data (ButtonTile.java:129-150)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;

	// Beta: ButtonTile.attack() - calls use() (ButtonTile.java:153-155)
	virtual void attack(Level &level, int_t x, int_t y, int_t z, Player &player) override;

	// Beta: ButtonTile.use() - toggles button state (ButtonTile.java:158-188)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;

	// Beta: ButtonTile.onRemove() - updates neighbors when removed (ButtonTile.java:191-210)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: ButtonTile.getSignal() - returns signal state (ButtonTile.java:213-215)
	virtual bool getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing) override;

	// Beta: ButtonTile.getDirectSignal() - returns direct signal (ButtonTile.java:218-234)
	virtual bool getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing) override;

	// Beta: ButtonTile.isSignalSource() returns true (ButtonTile.java:237-239)
	virtual bool isSignalSource() override;

	// Beta: ButtonTile.tick() - auto-releases button after delay (ButtonTile.java:242-265)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;

	// Beta: ButtonTile.updateDefaultShape() - sets default shape (ButtonTile.java:268-273)
	virtual void updateDefaultShape() override;

private:
	// Beta: ButtonTile.checkCanSurvive() - checks if button can survive (ButtonTile.java:118-126)
	bool checkCanSurvive(Level &level, int_t x, int_t y, int_t z);
};
