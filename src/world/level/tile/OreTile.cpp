#include "world/level/tile/OreTile.h"
#include "world/item/Item.h"
#include "world/item/Items.h"
#include "java/Random.h"

// Alpha: BlockOre.java
OreTile::OreTile(int_t id, int_t tex, int_t dropId) : Tile(id, tex, Material::stone), dropItemId(dropId)
{
	setDestroyTime(3.0f);  // Alpha: hardness 3.0F (Block.java:36-37, 78)
	// Alpha: resistance 5.0F (Block.java:36-38, 78, 95)
	// Note: setDestroyTime auto-sets resistance to 15.0F (3.0 * 5.0), but Block.java explicitly sets 5.0F
	setExplosionResistance(5.0f);  // Override to match Alpha resistance 5.0F
}

int_t OreTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockOre.idDropped() returns Item.coal.shiftedIndex for coal, Item.diamond.shiftedIndex for diamond, else blockID (BlockOre.java:10-11)
	if (dropItemId > 0)
		return dropItemId;  // Return item shiftedIndex (256 + itemID)
	return id;  // Return block ID
}
