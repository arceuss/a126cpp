#pragma once

#include "world/level/tile/Tile.h"

#include "java/Type.h"

// Beta 1.2 BookshelfTile.java
// Bookshelf block with custom texture logic (sides show wood, top/bottom show books)
class BookshelfTile : public Tile
{
public:
	BookshelfTile(int_t id, int_t texture);
	
	virtual int_t getTexture(Facing face) override;
	virtual int_t getResourceCount(Random &random) override;
};
