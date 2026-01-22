#include "BookshelfTile.h"

#include "Facing.h"
#include "java/Random.h"

BookshelfTile::BookshelfTile(int_t id, int_t texture) : Tile(id, texture, Material::wood)
{
	// Beta: Properties set in initTiles()
}

int_t BookshelfTile::getTexture(Facing face)
{
	// Beta: BookshelfTile.getTexture() (BookshelfTile.java:12-14)
	// Returns 4 (wood texture) for top/bottom (face <= 1), otherwise this.tex (books texture)
	// Java: DOWN=0, UP=1, so var1 <= 1 means DOWN or UP
	if (face <= Facing::UP)  // DOWN (0) or UP (1) - top and bottom faces
		return 4;  // Wood texture
	return this->tex;  // Books texture (35)
}

int_t BookshelfTile::getResourceCount(Random &random)
{
	// Beta: BookshelfTile.getResourceCount() returns 0 (BookshelfTile.java:17-19)
	// Bookshelves don't drop books in Beta 1.2
	return 0;
}
