#include "world/level/tile/ClayTile.h"
#include "world/level/material/Material.h"
#include "world/item/Items.h"
#include "java/Random.h"

// Beta: Tile.clay = new ClayTile(82, 72).setDestroyTime(0.6F).setSoundType(SOUND_GRAVEL)
// Alpha: Block.blockClay = new BlockClay(82, 72).setHardness(0.6F).setStepSound(soundGravelFootstep)
ClayTile::ClayTile(int_t id, int_t texture) : Tile(id, texture, Material::clay)
{
	setDestroyTime(0.6f);
	// Beta: setSoundType(SOUND_GRAVEL) - handled by Material.clay
}

int_t ClayTile::getResource(int_t data, Random &random)
{
	// Beta: ClayTile.getResource() returns Item.clay.id (ClayTile.java:13-15)
	// Note: Item.clay.id in Beta 1.2 is the shiftedIndex (256 + 81 = 337)
	// But getResource() should return the item ID that ItemStack can use
	// In Beta 1.2, Item.clay.id = 81, and ItemInstance uses shiftedIndex = 256 + 81
	// But spawnResources creates ItemInstance with the ID from getResource()
	// So we need to return the shiftedIndex
	return Items::clay != nullptr ? Items::clay->getShiftedIndex() : 0;
}

int_t ClayTile::getResourceCount(Random &random)
{
	// Beta: ClayTile.getResourceCount() returns 4
	// Alpha: BlockClay.quantityDropped() returns 4
	return 4;
}
