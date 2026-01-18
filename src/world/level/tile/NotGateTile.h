#pragma once

#include "world/level/tile/TorchTile.h"
#include <vector>

// Beta 1.2: NotGateTile.java - ID 75 (off), ID 76 (on), extends TorchTile
// Alpha 1.2.6: BlockRedstoneTorch - ID 75 (torchRedstoneIdle), ID 76 (torchRedstoneActive)
class NotGateTile : public TorchTile
{
private:
	static const int_t RECENT_TOGGLE_TIMER = 100;
	static const int_t MAX_RECENT_TOGGLES = 8;
	bool on;
	
	// Static list of recent toggles (shared across all instances)
	struct Toggle
	{
		int_t x, y, z;
		long_t when;
		Toggle(int_t x, int_t y, int_t z, long_t when) : x(x), y(y), z(z), when(when) {}
	};
	static std::vector<Toggle> recentToggles;

	// Beta: NotGateTile.isToggledTooFrequently() - checks toggle frequency (NotGateTile.java:20-37)
	bool isToggledTooFrequently(Level &level, int_t x, int_t y, int_t z, bool addToggle);
	
	// Beta: NotGateTile.hasNeighborSignal() - checks if neighbor has signal (NotGateTile.java:96-107)
	bool hasNeighborSignal(Level &level, int_t x, int_t y, int_t z);

public:
	NotGateTile(int_t id, int_t tex, bool on);
	
	// Beta: NotGateTile.getTexture() - returns texture based on face (NotGateTile.java:15-18)
	virtual int_t getTexture(Facing face, int_t data) override;
	
	// Beta: NotGateTile.getTickDelay() - returns 2 (NotGateTile.java:45-48)
	virtual int_t getTickDelay() override;
	
	// Beta: NotGateTile.onPlace() - updates neighbors if on (NotGateTile.java:50-64)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: NotGateTile.onRemove() - updates neighbors if on (NotGateTile.java:66-76)
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z) override;
	
	// Beta: NotGateTile.getSignal() - returns signal based on orientation (NotGateTile.java:78-94)
	virtual bool getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing) override;
	
	// Beta: NotGateTile.tick() - toggles based on neighbor signal (NotGateTile.java:109-134)
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	
	// Beta: NotGateTile.neighborChanged() - schedules tick (NotGateTile.java:136-140)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	
	// Beta: NotGateTile.getDirectSignal() - returns direct signal (NotGateTile.java:142-145)
	virtual bool getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing) override;
	
	// Beta: NotGateTile.getResource() - returns notGate_on (NotGateTile.java:147-150)
	virtual int_t getResource(int_t data, Random &random) override;
	
	// Beta: NotGateTile.isSignalSource() - returns true (NotGateTile.java:152-155)
	virtual bool isSignalSource() override;
	
	// Beta: NotGateTile.animateTick() - adds reddust particles if on (NotGateTile.java:157-178)
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
};
