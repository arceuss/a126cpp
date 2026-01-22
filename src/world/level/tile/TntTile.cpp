#include "TntTile.h"

#include "Facing.h"
#include "world/level/Level.h"
#include "java/Random.h"

TntTile::TntTile(int_t id, int_t texture) : Tile(id, texture, Material::explosive)
{
	// Beta: Properties set in initTiles()
}

int_t TntTile::getTexture(Facing face)
{
	// Beta: TntTile.getTexture() (TntTile.java:14-19)
	// Face 0 (DOWN) = tex+2, face 1 (UP) = tex+1, others = tex
	if (face == Facing::DOWN)  // Face 0
		return this->tex + 2;
	else if (face == Facing::UP)  // Face 1
		return this->tex + 1;
	else
		return this->tex;
}

void TntTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: TntTile.neighborChanged() (TntTile.java:23-28)
	// If neighbor is signal source and level has neighbor signal, destroy TNT
	// TODO: Implement Tile::isSignalSource() and Level::hasNeighborSignal() for redstone signal detection
	// For now, skip redstone trigger logic - TNT will activate when player breaks it
	// if (tile > 0)
	// {
	//     Tile *neighborTile = Tile::tiles[tile];
	//     if (neighborTile != nullptr && neighborTile->isSignalSource() && level.hasNeighborSignal(x, y, z))
	//     {
	//         this->destroy(level, x, y, z, 0);
	//         level.setTile(x, y, z, 0);
	//     }
	// }
}

int_t TntTile::getResourceCount(Random &random)
{
	// Beta: TntTile.getResourceCount() returns 0 (TntTile.java:31-33)
	// TNT doesn't drop when broken (converted to PrimedTnt entity)
	return 0;
}

void TntTile::destroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	// Beta: TntTile.destroy() (TntTile.java:43-49)
	// Creates PrimedTnt entity if not online, plays fuse sound
	if (!level.isOnline)
	{
		// TODO: Implement PrimedTnt entity class
		// PrimedTnt *primedTnt = new PrimedTnt(level, x + 0.5f, y + 0.5f, z + 0.5f);
		// level.addEntity(primedTnt);
		// level.playSound(primedTnt, "random.fuse", 1.0f, 1.0f);
	}
}
