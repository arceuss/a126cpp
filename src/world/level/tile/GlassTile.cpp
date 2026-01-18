#include "GlassTile.h"

#include "java/Random.h"

GlassTile::GlassTile(int_t id, int_t texture, const Material &material, bool allowSame) 
	: TransparentTile(id, texture, material, allowSame)
{
	// Beta: Properties set in initTiles()
}

int_t GlassTile::getResourceCount(Random &random)
{
	// Beta: GlassTile.getResourceCount() returns 0 (glass doesn't drop) (GlassTile.java:12-14)
	return 0;
}
