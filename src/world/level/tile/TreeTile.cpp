#include "world/level/tile/TreeTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"

TreeTile::TreeTile(int_t id) : Tile(id, Material::wood)
{
	// Beta: TreeTile constructor sets tex = 20 (TreeTile.java:13)
	tex = 20;
}

int_t TreeTile::getResourceCount(Random &random)
{
	return 1;
}

int_t TreeTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockLog.idDropped() returns Block.wood.blockID (BlockLog.java:15-16)
	// Note: In Alpha, Block.wood IS the log (BlockLog ID 17), not planks!
	// Block.java:39: public static final Block wood = (new BlockLog(17))
	// So logs drop themselves (ID 17), not planks (ID 5)
	// Beta: TreeTile.getResource() returns Tile.treeTrunk.id (TreeTile.java:22-24)
	return id;  // Return log itself (ID 17)
}

void TreeTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	if (level.isOnline)
		return;

	// Update leaves? TODO
}

int_t TreeTile::getTexture(Facing face, int_t data)
{
	// Alpha 1.2.6: BlockLog.getBlockTextureFromSide() - only checks face, ignores data
	// Returns 21 for top/bottom (face 0=DOWN, 1=UP), 20 for sides
	// Alpha: return var1 == 1 ? 21 : (var1 == 0 ? 21 : 20);
	return (face == Facing::UP || face == Facing::DOWN) ? 21 : 20;
}

int_t TreeTile::getSpawnResourcesAuxValue(int_t data)
{
	return data;
}
