#include "world/level/tile/TntTile.h"

#include <memory>

#include "world/entity/PrimedTnt.h"
#include "world/level/Level.h"

TntTile::TntTile(int_t id, int_t texture) : Tile(id, texture, Material::explosive)
{
}

int_t TntTile::getTexture(Facing face)
{
	if (face == Facing::DOWN)
		return tex + 2;
	if (face == Facing::UP)
		return tex + 1;
	return tex;
}

void TntTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Direct Alpha transliteration: BlockTNT.java:28-32.
	if (tile > 0 && Tile::tiles[tile]->isSignalSource()
		&& level.hasNeighborSignal(x, y, z))
	{
		destroy(level, x, y, z, 0);
		level.setTile(x, y, z, 0);
	}
}

int_t TntTile::getResourceCount(Random &random)
{
	(void)random;
	return 0;
}

void TntTile::wasExploded(Level &level, int_t x, int_t y, int_t z)
{
	// Direct Alpha transliteration: BlockTNT.java:39-42.
	std::shared_ptr<PrimedTnt> primed = std::make_shared<PrimedTnt>(
		level, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
		static_cast<double>(z) + 0.5);
	primed->life = level.random.nextInt(primed->life / 4) + primed->life / 8;
	level.addEntity(primed);
}

void TntTile::destroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	(void)data;
	// Direct Alpha transliteration: BlockTNT.java:45-52.
	if (level.isOnline)
		return;
	std::shared_ptr<PrimedTnt> primed = std::make_shared<PrimedTnt>(
		level, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
		static_cast<double>(z) + 0.5);
	level.addEntity(primed);
	level.playSound(primed.get(), u"random.fuse", 1.0f, 1.0f);
}
