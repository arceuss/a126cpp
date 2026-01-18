#include "ClothTile.h"

// Alpha 1.2.6 Block.cloth (Block.java:57)
// public static final Block cloth = (new Block(35, 64, Material.cloth)).setHardness(0.8F).setStepSound(soundClothFootstep);
ClothTile::ClothTile() : Tile(35, 64, Material::cloth)
{
	// Alpha: Properties set in initTiles()
	// hardness = 0.8F, stepSound = soundClothFootstep (SOUND_CLOTH)
}
