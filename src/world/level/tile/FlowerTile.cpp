#include "world/level/tile/FlowerTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/FarmTile.h"
#include "java/Random.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "Facing.h"

// Alpha: BlockFlower.java
FlowerTile::FlowerTile(int_t id, int_t tex) : TransparentTile(id, tex, Material::getPlantsMaterial(), false)
{
	// Alpha: BlockFlower sets bounds (0.5 - 0.2, 0, 0.5 - 0.2, 0.5 + 0.2, 0.2 * 3, 0.5 + 0.2)
	// = (0.3, 0, 0.3, 0.7, 0.6, 0.7)
	float size = 0.2f;
	setShape(0.5f - size, 0.0f, 0.5f - size, 0.5f + size, size * 3.0f, 0.5f + size);
	
	setTicking(true);
	setDestroyTime(0.0f);  // Alpha: hardness 0.0F (Block.java:59-60)
}

bool FlowerTile::mayPick()
{
	return true;
}

bool FlowerTile::canThisPlantGrowOnThisBlockID(int_t blockId)
{
	// Alpha: BlockFlower.canThisPlantGrowOnThisBlockID() returns true for grass, dirt, tilledField (BlockFlower.java:18-19)
	// Beta: Bush.mayPlaceOn() returns true for grass, dirt, farmland (Bush.java:23)
	return blockId == Tile::grass.id || blockId == Tile::dirt.id || blockId == Tile::farmland.id;
}

bool FlowerTile::canBlockStay(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha: BlockFlower.canBlockStay() checks light >= 8 or canSeeSky, and valid block below (BlockFlower.java:39-40)
	int_t lightValue = level.getRawBrightness(x, y, z);
	bool hasLight = lightValue >= 8 || level.canSeeSky(x, y, z);
	int_t belowId = level.getTile(x, y - 1, z);
	return hasLight && canThisPlantGrowOnThisBlockID(belowId);
}

void FlowerTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Alpha: BlockFlower.updateTick() calls func_268_h which checks canBlockStay and drops if invalid (BlockFlower.java:27-35)
	if (!canBlockStay(level, x, y, z))
	{
		spawnResources(level, x, y, z, 0);
		level.setTile(x, y, z, 0);
	}
}

Tile::Shape FlowerTile::getRenderShape()
{
	// Alpha: BlockFlower.getRenderType() returns 1 (cross-texture rendering) (BlockFlower.java:55-57)
	return SHAPE_CROSS_TEXTURE;
}

bool FlowerTile::isCubeShaped()
{
	// Alpha: BlockFlower.renderAsNormalBlock() returns false (BlockFlower.java:51-53)
	return false;
}

AABB *FlowerTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha: BlockFlower.getCollisionBoundingBoxFromPool() returns null (no collision) (BlockFlower.java:43-45)
	// Also need to override getAABB() since addAABBs() calls this
	return nullptr;
}

bool FlowerTile::canPlaceBlockAt(Level &level, int_t x, int_t y, int_t z)
{
	// Alpha: BlockFlower.canPlaceBlockAt() checks canThisPlantGrowOnThisBlockID (BlockFlower.java:14-16)
	return canThisPlantGrowOnThisBlockID(level.getTile(x, y - 1, z));
}

void FlowerTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Alpha: BlockFlower.onNeighborBlockChange() calls func_268_h (validation) (BlockFlower.java:22-25)
	Tile::neighborChanged(level, x, y, z, tile);
	// func_268_h is equivalent to the validation check in tick()
	if (!canBlockStay(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
	}
}

HitResult FlowerTile::clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to)
{
	// Alpha: Plants have no collision box but should still be selectable
	// getSelectedBoundingBoxFromPool() uses block bounds for selection
	// The shape bounds are already set correctly in the constructor (0.3, 0, 0.3, 0.7, 0.6, 0.7)
	// Don't call updateShape() as it might reset our custom bounds - plants have fixed bounds
	// updateShape(level, x, y, z); // Skip for plants
	
	Vec3 *localFrom = from.add(-x, -y, -z);
	Vec3 *localTo = to.add(-x, -y, -z);
	Vec3 *cxx0 = localFrom->clipX(*localTo, xx0);
	Vec3 *cxx1 = localFrom->clipX(*localTo, xx1);
	Vec3 *cyy0 = localFrom->clipY(*localTo, yy0);
	Vec3 *cyy1 = localFrom->clipY(*localTo, yy1);
	Vec3 *czz0 = localFrom->clipZ(*localTo, zz0);
	Vec3 *czz1 = localFrom->clipZ(*localTo, zz1);

	// Check bounds validity before using containsX/Y/Z
	if (cxx0 != nullptr && !containsX(cxx0)) cxx0 = nullptr;
	if (cxx1 != nullptr && !containsX(cxx1)) cxx1 = nullptr;
	if (cyy0 != nullptr && !containsY(cyy0)) cyy0 = nullptr;
	if (cyy1 != nullptr && !containsY(cyy1)) cyy1 = nullptr;
	if (czz0 != nullptr && !containsZ(czz0)) czz0 = nullptr;
	if (czz1 != nullptr && !containsZ(czz1)) czz1 = nullptr;

	Vec3 *pick = nullptr;
	if (cxx0 != nullptr && (pick == nullptr || localFrom->distanceTo(*cxx0) < localFrom->distanceTo(*pick))) pick = cxx0;
	if (cxx1 != nullptr && (pick == nullptr || localFrom->distanceTo(*cxx1) < localFrom->distanceTo(*pick))) pick = cxx1;
	if (cyy0 != nullptr && (pick == nullptr || localFrom->distanceTo(*cyy0) < localFrom->distanceTo(*pick))) pick = cyy0;
	if (cyy1 != nullptr && (pick == nullptr || localFrom->distanceTo(*cyy1) < localFrom->distanceTo(*pick))) pick = cyy1;
	if (czz0 != nullptr && (pick == nullptr || localFrom->distanceTo(*czz0) < localFrom->distanceTo(*pick))) pick = czz0;
	if (czz1 != nullptr && (pick == nullptr || localFrom->distanceTo(*czz1) < localFrom->distanceTo(*pick))) pick = czz1;
	
	if (pick == nullptr)
		return HitResult();

	Facing face = Facing::NONE;
	if (pick == cxx0)
		face = Facing::WEST;
	else if (pick == cxx1)
		face = Facing::EAST;
	else if (pick == cyy0)
		face = Facing::DOWN;
	else if (pick == cyy1)
		face = Facing::UP;
	else if (pick == czz0)
		face = Facing::NORTH;
	else if (pick == czz1)
		face = Facing::SOUTH;

	// CRASH GUARD: Safe null check before calling add() - prevents crashes when looking at flowers
	// TODO: Full fix requires renderer support for SHAPE_CROSS_TEXTURE (flowers invisible currently)
	// Suspected code path: FlowerTile::clip() -> Vec3::add() -> HitResult construction
	// Vec3::add() may return null if pool is exhausted - check and return empty hit
	Vec3 *worldPos = pick->add(x, y, z);
	if (worldPos == nullptr)
		return HitResult();
	
	return HitResult(x, y, z, face, *worldPos);
}
