#include "world/level/tile/StoneSlabTile.h"
#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "Facing.h"
#include "java/Random.h"

// Beta: Tile.stoneSlab = new StoneSlabTile(43, true).setDestroyTime(2.0F).setExplodeable(10.0F).setSoundType(SOUND_STONE)
// Beta: Tile.stoneSlabHalf = new StoneSlabTile(44, false).setDestroyTime(2.0F).setExplodeable(10.0F).setSoundType(SOUND_STONE)
StoneSlabTile::StoneSlabTile(int_t id, bool fullSize) : Tile(id, 6, Material::stone), fullSize(fullSize)
{
	// Beta: Set shape for half slab (StoneSlabTile.java:14-16)
	if (!fullSize)
	{
		setShape(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F)
		// Beta: Half slabs need lightBlock = 0 to allow light through upper half
		// Special handling in Level.getRawBrightness() (Level.java:603-626) ensures correct brightness calculation
		setLightBlock(0);  // Allow light to propagate through half slabs
	}
	else
	{
		// Beta: Full slabs block all light (StoneSlabTile.java:18)
		setLightBlock(255);
	}
	setDestroyTime(2.0f);  // Beta: setDestroyTime(2.0F)
	setExplosionResistance(10.0f);  // Beta: setExplodeable(10.0F)
	// Sound type is handled by Material::stone
}

int_t StoneSlabTile::getTexture(Facing face, int_t data)
{
	// Beta: StoneSlabTile.getTexture() - different textures for top/bottom vs sides (StoneSlabTile.java:22-24)
	if (face == Facing::UP || face == Facing::DOWN)  // Face <= 1
	{
		return 6;  // Beta: return 6 (top/bottom texture)
	}
	else
	{
		return 5;  // Beta: return 5 (side texture)
	}
}

bool StoneSlabTile::isSolidRender()
{
	// Beta: StoneSlabTile.isSolidRender() - returns fullSize (StoneSlabTile.java:27-29)
	return fullSize;
}

bool StoneSlabTile::isCubeShaped()
{
	// Beta: StoneSlabTile.isCubeShaped() - returns fullSize (StoneSlabTile.java:57-59)
	return fullSize;
}

void StoneSlabTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: StoneSlabTile.neighborChanged() - empty for half slab (StoneSlabTile.java:32-36)
	// Empty implementation for half slab
}

void StoneSlabTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StoneSlabTile.onPlace() - combines with half slab below to make double (StoneSlabTile.java:39-49)
	if (this != &Tile::stoneSlabHalf)
	{
		Tile::onPlace(level, x, y, z);  // Beta: super.onPlace(var1, var2, var3, var4)
	}
	
	// Beta: Check if half slab below, combine to make double (StoneSlabTile.java:44-48)
	int_t belowTile = level.getTile(x, y - 1, z);  // Beta: var5 = var1.getTile(var2, var3 - 1, var4)
	if (belowTile == Tile::stoneSlabHalf.id)
	{
		level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0)
		level.setTile(x, y - 1, z, Tile::stoneSlab.id);  // Beta: var1.setTile(var2, var3 - 1, var4, Tile.stoneSlab.id)
	}
}

int_t StoneSlabTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockStep.idDropped() returns Block.stairSingle.blockID (BlockStep.java:44-45)
	// Note: Alpha calls it "stairSingle" but it's the half slab (ID 44)
	// Beta: StoneSlabTile.getResource() - double slab drops half slab (StoneSlabTile.java:52-54)
	return Tile::stoneSlabHalf.id;  // Alpha/Beta: return half slab ID
}

bool StoneSlabTile::shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: StoneSlabTile.shouldRenderFace() - special face rendering logic (StoneSlabTile.java:62-74)
	if (this != &Tile::stoneSlabHalf)
	{
		return Tile::shouldRenderFace(level, x, y, z, face);  // Beta: return super.shouldRenderFace(var1, var2, var3, var4, var5) (StoneSlabTile.java:64)
	}
	
	if (face == Facing::UP)  // Face 1 (StoneSlabTile.java:67)
	{
		return true;  // Beta: return true (StoneSlabTile.java:68)
	}
	else if (!Tile::shouldRenderFace(level, x, y, z, face))  // Beta: else if (!super.shouldRenderFace(...)) (StoneSlabTile.java:69)
	{
		return false;  // Beta: return false (StoneSlabTile.java:70)
	}
	else
	{
		// Beta: return var5 == 0 ? true : var1.getTile(var2, var3, var4) != this.id (StoneSlabTile.java:72)
		// Note: x, y, z are already the neighbor's coordinates (passed from TileRenderer)
		if (face == Facing::DOWN)  // Face 0
			return true;
		// Beta: Check if neighbor block is not the same half slab ID
		// For side faces, show the face if the neighbor is not the same half slab
		int_t neighborTile = level.getTile(x, y, z);
		return neighborTile != id;
	}
}
