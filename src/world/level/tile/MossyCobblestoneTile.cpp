#include "world/level/tile/MossyCobblestoneTile.h"
#include "world/level/material/Material.h"

// Beta: Tile.mossStone = new Tile(48, 36, Material.stone).setDestroyTime(2.0F).setExplodeable(10.0F)
MossyCobblestoneTile::MossyCobblestoneTile(int_t id, int_t texture) : Tile(id, texture, Material::stone)
{
	// Beta: hardness 2.0F, resistance 10.0F
	setDestroyTime(2.0f);
	setExplodeable(10.0f);
}
