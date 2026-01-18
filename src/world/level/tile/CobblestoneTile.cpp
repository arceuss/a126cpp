#include "CobblestoneTile.h"

CobblestoneTile::CobblestoneTile(int_t id, int_t texture) : Tile(id, texture, Material::stone)
{
	// Alpha: Material.rock has setNoHarvest() (Material.java:7)
	// This means cobblestone requires a pickaxe to harvest (canHarvestBlock returns false for hand)
}
