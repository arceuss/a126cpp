#pragma once

#include "world/level/tile/Tile.h"

// Beta 1.2: RecordPlayerTile.java - ID 84, texture 85
// Alpha: Block.jukebox (Block.java:107)
// RecordPlayerTile uses Material.wood, plays music records
class RecordPlayerTile : public Tile
{
public:
	RecordPlayerTile(int_t id, int_t texture);
	
	// Beta: RecordPlayerTile.getTexture() - different texture for top face (RecordPlayerTile.java:16-18)
	virtual int_t getTexture(Facing face) override;
	
	// Beta: RecordPlayerTile.use() - ejects record if present (RecordPlayerTile.java:21-29)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player) override;
	
	// Beta: RecordPlayerTile.spawnResources() - drops record if present (RecordPlayerTile.java:45-53)
	virtual void spawnResources(Level &level, int_t x, int_t y, int_t z, int_t data, float chance) override;
	
protected:
	// Beta: RecordPlayerTile.dropRecording() - ejects record and stops music (RecordPlayerTile.java:31-42)
	void dropRecording(Level &level, int_t x, int_t y, int_t z, int_t recordData);
};
