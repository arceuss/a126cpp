#pragma once

#include "world/level/tile/Tile.h"

class SandTile : public Tile
{
public:
	static bool fallInstantly;

	SandTile(int_t id, int_t tex);

	void onPlace(Level &level, int_t x, int_t y, int_t z) override;
	void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;
	void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	int_t getTickDelay() override;

	static bool canFallBelow(Level &level, int_t x, int_t y, int_t z);

private:
	void tryToFall(Level &level, int_t x, int_t y, int_t z);
};
