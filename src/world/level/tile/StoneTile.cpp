#include "StoneTile.h"
#include "world/level/tile/CobblestoneTile.h"

StoneTile::StoneTile(int_t id, int_t texture) : Tile(id, texture, Material::stone)
{
	// Alpha: Material.rock has setNoHarvest() (Material.java:7)
	// This means stone requires a pickaxe to harvest (canHarvestBlock returns false for hand)
}

int_t StoneTile::getResource(int_t subtype, Random &random)
{
	// Beta: StoneTile.getResource() returns Tile.stoneBrick.id (StoneTile.java:12-14)
	// In Beta, stoneBrick is ID 4 (cobblestone)
	return Tile::cobblestone.id;
}
