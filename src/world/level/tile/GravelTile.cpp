#include "world/level/tile/GravelTile.h"
#include "world/item/Items.h"

GravelTile::GravelTile(int_t id, int_t tex) : SandTile(id, tex)
{
	
}

int_t GravelTile::getResource(int_t data, Random &random)
{
	// Alpha: Gravel has 10% chance (1/10) to drop flint, otherwise drops itself (BlockGravel.java:10-11)
	// Flint is Item ID 62, which becomes shiftedIndex 256+62=318
	if (random.nextInt(10) == 0) {
		// Return flint item shiftedIndex (256 + 62 = 318)
		return Items::flint != nullptr ? Items::flint->getShiftedIndex() : id;
	}
	return id;
}