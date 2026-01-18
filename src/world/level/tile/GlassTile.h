#pragma once

#include "world/level/tile/TransparentTile.h"

#include "java/Type.h"

// Beta 1.2 GlassTile.java
// Extends HalfTransparentTile (TransparentTile in C++), glass doesn't drop when broken
class GlassTile : public TransparentTile
{
public:
	GlassTile(int_t id, int_t texture, const Material &material, bool allowSame);
	
	virtual int_t getResourceCount(Random &random) override;
};
