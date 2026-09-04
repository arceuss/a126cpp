#include "world/level/tile/LeafTile.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/FoliageColor.h"
#include "world/level/tile/SaplingTile.h"
#include "java/Random.h"
#include "world/entity/Entity.h"

// Alpha 1.2.6: BlockLeaves never calls setTickOnLoad (Block.java:40), and its
// updateTick/onNeighborBlockChange bodies are dead (`if (this == null)`,
// BlockLeaves.java:23-29,90-102), so leaves neither tick nor decay. The Beta
// decay this port used to run rewrote ~1,600 leaf blocks per 200 frames at
// spawn, which dirtied three quarters of the sections rebuilt at idle.
LeafTile::LeafTile(int_t id, int_t tex) : TransparentTile(id, tex, Material::leaves, false)  // Beta: Material.leaves (LeafTile.java:21)
{
	oTex = tex;
}

int_t LeafTile::getColor(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Alpha 1.2.6: BlockLeaves.colorMultiplier() - only uses biome temperature/humidity
	// No leaf type variants - only oak leaves exist
	// Alpha: var1.func_4075_a().func_4069_a(var2, var4, 1, 1);
	//        double var5 = var1.func_4075_a().temperature[0];
	//        double var7 = var1.func_4075_a().humidity[0];
	//        return ColorizerFoliage.func_4146_a(var5, var7);
	level.getBiomeSource().getBiomeBlock(x, z, 1, 1);
	double temperature = level.getBiomeSource().temperatures[0];
	double downfall = level.getBiomeSource().downfalls[0];

	return FoliageColor::get(temperature, downfall);
}

int_t LeafTile::getResourceCount(Random &random)
{
	// Alpha: BlockLeaves.quantityDropped() - 1/20 chance (5%) for sapling (BlockLeaves.java:108-110)
	// Beta uses 1/16, but Alpha uses 1/20
	return (random.nextInt(20) == 0) ? 1 : 0;
}

int_t LeafTile::getResource(int_t data, Random &random)
{
	// Beta: LeafTile.getResource() returns Tile.sapling.id (LeafTile.java:146-148)
	return Tile::sapling.id;
}

bool LeafTile::isSolidRender()
{
	return !allowSame;
}

int_t LeafTile::getTexture(Facing face, int_t data)
{
	// Alpha 1.2.6: BlockLeaves.getBlockTextureFromSide() is inherited from Block
	// Only uses baseIndexInPNG (tex) which changes with setGraphicsLevel() (fancy/non-fancy)
	// No type variants - only oak leaves exist, metadata is only for decay checking
	// Alpha: setGraphicsLevel() sets blockIndexInTexture = baseIndexInPNG + (var1 ? 0 : 1)
	return tex;
}

void LeafTile::setFancy(bool fancy)
{
	allowSame = fancy;
	tex = oTex + (fancy ? 0 : 1);
}

void LeafTile::stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	Tile::stepOn(level, x, y, z, entity);
}
