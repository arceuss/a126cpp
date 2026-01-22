#include "world/level/tile/LightGemTile.h"
#include "world/item/Items.h"
#include "java/Random.h"

// Beta: LightGemTile.java
LightGemTile::LightGemTile(int_t id, int_t texture, const Material &material) : Tile(id, texture, material)
{
	// Beta: Properties set in initTiles()
}

int_t LightGemTile::getResource(int_t data, Random &random)
{
	// Beta: LightGemTile.getResource() (LightGemTile.java:13-15)
	// Returns Item.yellowDust.id
	if (Items::yellowDust != nullptr)
		return Items::yellowDust->getShiftedIndex();
	return 0;
}
