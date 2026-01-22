#include "ObsidianTile.h"

ObsidianTile::ObsidianTile(int_t id, int_t texture) : StoneTile(id, texture)
{
	// Alpha: Obsidian extends BlockStone, so it uses Material.rock
	// Hardness and resistance are set in initTiles() (10.0F, 2000.0F)
}

int_t ObsidianTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockObsidian.idDropped() returns Block.obsidian.blockID (BlockObsidian.java:15)
	// Obsidian always drops itself (ID 49)
	return id;
}
