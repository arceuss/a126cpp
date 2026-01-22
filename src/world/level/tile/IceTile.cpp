#include "world/level/tile/IceTile.h"

#include "world/level/Level.h"
#include "world/level/LightLayer.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "java/Random.h"

// Alpha: BlockIce.java
IceTile::IceTile(int_t id, int_t tex) : TransparentTile(id, tex, Material::ice, false)
{
	// Alpha: BlockIce constructor (BlockIce.java:6-10)
	// - Material.ice
	// - Slipperiness: 0.98F (BlockIce.java:8)
	// - setTickOnLoad(true) (BlockIce.java:9)
	// - Hardness: 0.5F (Block.java:101)
	// - Light opacity: 3 (Block.java:101)
	// - Step sound: soundGlassFootstep (Block.java:101)
	setDestroyTime(0.5f);  // Alpha: hardness 0.5F
	setLightBlock(3);      // Alpha: lightOpacity 3
	setTicking(true);      // Alpha: setTickOnLoad(true)
	
	// Note: Slipperiness is handled by Entity movement code, not in Tile
	// Note: Step sound is handled by Tile base class sound system
}

// Alpha: BlockIce.quantityDropped() returns 0 (BlockIce.java:28-30)
int_t IceTile::getResource(int_t data, Random &random)
{
	return 0;  // Alpha: Ice drops nothing when broken
}

// Alpha: BlockIce.updateTick() - melts when light > 11 - lightOpacity (BlockIce.java:32-37)
void IceTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Alpha: if(var1.getSavedLightValue(EnumSkyBlock.Block, var2, var3, var4) > 11 - Block.lightOpacity[this.blockID])
	// Alpha: lightOpacity[blockIce] = 3, so melts when block light > 11 - 3 = 8
	// Alpha: EnumSkyBlock.Block = block light (torch light), not sky light
	int_t lightValue = level.getBrightness(LightLayer::Block, x, y, z);
	int_t lightThreshold = 11 - Tile::lightBlock[id];
	
	if (lightValue > lightThreshold)
	{
		// Alpha: this.dropBlockAsItem(var1, var2, var3, var4, var1.getBlockMetadata(var2, var3, var4));
		// Alpha: var1.setBlockWithNotify(var2, var3, var4, Block.waterMoving.blockID);
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, Tile::calmWater.id);  // Alpha: Block.waterMoving.blockID (ID 9) - setTile takes 4 args
	}
}

// Alpha: BlockIce.onBlockRemoval() - converts to water if block below is solid/liquid (BlockIce.java:20-26)
void IceTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha: Material var5 = var1.getBlockMaterial(var2, var3 - 1, var4);
	// Alpha: if(var5.func_880_c() || var5.getIsLiquid())
	// Alpha: func_880_c() = isSolid() (Material.java:43-45)
	const Material &belowMat = level.getMaterial(x, y - 1, z);
	if (belowMat.isSolid() || belowMat.isLiquid())
	{
		// Alpha: var1.setBlockWithNotify(var2, var3, var4, Block.waterStill.blockID);
		level.setTile(x, y, z, Tile::water.id);  // Alpha: Block.waterStill.blockID (ID 8)
	}
	
	Tile::onRemove(level, x, y, z);
}

// Alpha: BlockIce.shouldSideBeRendered() - reverses face for transparent rendering (BlockIce.java:16-18)
bool IceTile::shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Alpha: return super.shouldSideBeRendered(var1, var2, var3, var4, 1 - var5);
	// Alpha: Reverses the face direction for transparent rendering
	Facing reversedFace = Facing::NONE;
	switch (face)
	{
		case Facing::DOWN:  reversedFace = Facing::UP;    break;
		case Facing::UP:    reversedFace = Facing::DOWN;  break;
		case Facing::NORTH: reversedFace = Facing::SOUTH; break;
		case Facing::SOUTH: reversedFace = Facing::NORTH; break;
		case Facing::WEST:  reversedFace = Facing::EAST;  break;
		case Facing::EAST:  reversedFace = Facing::WEST;  break;
		default: reversedFace = face; break;
	}
	
	return Tile::shouldRenderFace(level, x, y, z, reversedFace);
}

// Alpha: Ice renders in translucent pass (pass 1) like water
int_t IceTile::getRenderLayer()
{
	return 1;  // Alpha: Translucent pass (same as water)
}
