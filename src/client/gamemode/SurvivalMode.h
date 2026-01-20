#pragma once

#include "client/gamemode/GameMode.h"

class SurvivalMode : public GameMode
{
private:
	int_t xDestroyBlock = -1;
	int_t yDestroyBlock = -1;
	int_t zDestroyBlock = -1;

	float destroyProgress = 0.0f;
	float oDestroyProgress = 0.0f;
	float destroyTicks = 0.0f;
	int_t destroyDelay = 0;
	
	// Rumble for block breaking (Controlify-style)
	float blockBreakRumbleStrength = 0.0f;  // Current rumble strength (0.0 = stopped)
	int_t blockBreakRumbleTicks = 0;  // Ticks since starting to break

public:
	SurvivalMode(Minecraft &minecraft);

	void initPlayer(std::shared_ptr<Player> player) override;

	void init();

	bool destroyBlock(int_t x, int_t y, int_t z, Facing face) override;
	void startDestroyBlock(int_t x, int_t y, int_t z, Facing face) override;
	void stopDestroyBlock() override;
	void continueDestroyBlock(int_t x, int_t y, int_t z, Facing face) override;

	void render(float a) override;

	float getPickRange() override;

	void initLevel(std::shared_ptr<Level> level) override;

	void tick() override;

private:
	// Rumble helpers for block breaking (Controlify-style)
	void startBlockBreakRumble(int_t x, int_t y, int_t z);
	void updateBlockBreakRumble();  // Call every tick while breaking
	void stopBlockBreakRumble();
};
