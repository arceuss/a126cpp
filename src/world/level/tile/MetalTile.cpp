#include "MetalTile.h"

MetalTile::MetalTile(int_t id, int_t texture) : Tile(id, Material::metal)
{
	this->tex = texture;
}

int_t MetalTile::getTexture(Facing face)
{
	// Beta: MetalTile.getTexture() always returns this.tex (MetalTile.java:12-14)
	return tex;
}
