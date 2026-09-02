#include "client/renderer/culling/OcclusionCuller.h"

#include <cmath>
#include <cstdlib>

#include "util/Mth.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"

bool OcclusionCuller::enabled = true;

// The box is widened by this much before it is split into voxels, so a mob
// standing against a wall still gets sample points on the open side.
static const double BOX_EXPANSION = 0.5;
// Boxes larger than this on any axis are drawn without a test.
static const double BOX_LIMIT = 50.0;
// Ticks an entity found visible is drawn without being tested again.
static const int_t VISIBLE_HOLD_TICKS = 2;

OcclusionCuller::OcclusionCuller()
{
	const std::size_t side = static_cast<std::size_t>(2 * REACH);
	cache.assign(side * side * side / 4, 0);
}

void OcclusionCuller::beginPass(Level &level, double x, double y, double z, int_t tick)
{
	const int_t cx = Mth::floor(x);
	const int_t cy = Mth::floor(y);
	const int_t cz = Mth::floor(z);
	if (&level != this->level || tick != passTick || cx != cameraX || cy != cameraY || cz != cameraZ)
	{
		resetCache();
		passId++;
	}
	this->level = &level;
	passTick = tick;
	viewerX = x;
	viewerY = y;
	viewerZ = z;
	cameraX = cx;
	cameraY = cy;
	cameraZ = cz;
}

bool OcclusionCuller::shouldRender(OcclusionCullState &state, const AABB &box)
{
	if (!enabled)
		return true;
	if (state.pass == passId)
		return !state.occluded;
	state.pass = passId;
	if (state.visibleUntilTick > passTick)
	{
		state.occluded = false;
		return true;
	}
	state.occluded = !isVisible(box);
	if (!state.occluded)
		state.visibleUntilTick = passTick + VISIBLE_HOLD_TICKS;
	return !state.occluded;
}

// Where the viewer's block lies relative to a span of voxels on one axis.
enum class Relative
{
	Inside,
	Positive,
	Negative
};

static Relative relativeTo(int_t min, int_t max, int_t pos)
{
	if (min > pos && max > pos)
		return Relative::Positive;
	if (min < pos && max < pos)
		return Relative::Negative;
	return Relative::Inside;
}

bool OcclusionCuller::isVisible(const AABB &box)
{
	if (box.x1 - box.x0 > BOX_LIMIT || box.y1 - box.y0 > BOX_LIMIT || box.z1 - box.z0 > BOX_LIMIT)
		return true;

	const int_t minX = Mth::floor(box.x0 - BOX_EXPANSION);
	const int_t minY = Mth::floor(box.y0 - BOX_EXPANSION);
	const int_t minZ = Mth::floor(box.z0 - BOX_EXPANSION);
	const int_t maxX = Mth::floor(box.x1 + BOX_EXPANSION);
	const int_t maxY = Mth::floor(box.y1 + BOX_EXPANSION);
	const int_t maxZ = Mth::floor(box.z1 + BOX_EXPANSION);

	const Relative relX = relativeTo(minX, maxX, cameraX);
	const Relative relY = relativeTo(minY, maxY, cameraY);
	const Relative relZ = relativeTo(minZ, maxZ, cameraZ);
	if (relX == Relative::Inside && relY == Relative::Inside && relZ == Relative::Inside)
		return true;

	// The cache only spans the traced reach; anything reaching past it is drawn.
	const int_t limit = REACH - 2;
	if (std::abs(minX - cameraX) > limit || std::abs(maxX - cameraX) > limit ||
		std::abs(minY - cameraY) > limit || std::abs(maxY - cameraY) > limit ||
		std::abs(minZ - cameraZ) > limit || std::abs(maxZ - cameraZ) > limit)
		return true;

	chunk = nullptr;

	const std::size_t voxels = static_cast<std::size_t>(maxX - minX + 1) *
		static_cast<std::size_t>(maxY - minY + 1) * static_cast<std::size_t>(maxZ - minZ + 1);
	skipList.assign(voxels, 0);

	// Answer from the cache when an earlier box this pass settled a voxel.
	std::size_t id = 0;
	for (int_t x = minX; x <= maxX; x++)
	{
		for (int_t y = minY; y <= maxY; y++)
		{
			for (int_t z = minZ; z <= maxZ; z++)
			{
				const int_t cached = getCacheValue(x, y, z);
				if (cached == CACHE_VISIBLE)
					return true;
				if (cached == CACHE_HIDDEN)
					skipList[id] = 1;
				id++;
			}
		}
	}

	// Rays may only be skipped against a wall this box has already hit.
	allowRayChecks = false;

	id = 0;
	for (int_t x = minX; x <= maxX; x++)
	{
		int_t faceEdgeX = 0;
		int_t visibleOnFaceX = 0;
		if (x == minX)
		{
			faceEdgeX |= ON_MIN_X;
			if (relX == Relative::Positive)
				visibleOnFaceX |= ON_MIN_X;
		}
		if (x == maxX)
		{
			faceEdgeX |= ON_MAX_X;
			if (relX == Relative::Negative)
				visibleOnFaceX |= ON_MAX_X;
		}
		for (int_t y = minY; y <= maxY; y++)
		{
			int_t faceEdgeY = faceEdgeX;
			int_t visibleOnFaceY = visibleOnFaceX;
			if (y == minY)
			{
				faceEdgeY |= ON_MIN_Y;
				if (relY == Relative::Positive)
					visibleOnFaceY |= ON_MIN_Y;
			}
			if (y == maxY)
			{
				faceEdgeY |= ON_MAX_Y;
				if (relY == Relative::Negative)
					visibleOnFaceY |= ON_MAX_Y;
			}
			for (int_t z = minZ; z <= maxZ; z++)
			{
				int_t faceEdge = faceEdgeY;
				int_t visibleOnFace = visibleOnFaceY;
				if (z == minZ)
				{
					faceEdge |= ON_MIN_Z;
					if (relZ == Relative::Positive)
						visibleOnFace |= ON_MIN_Z;
				}
				if (z == maxZ)
				{
					faceEdge |= ON_MAX_Z;
					if (relZ == Relative::Negative)
						visibleOnFace |= ON_MAX_Z;
				}
				if (skipList[id] == 0 && visibleOnFace != 0 && isVoxelVisible(x, y, z, faceEdge, visibleOnFace))
					return true;
				id++;
			}
		}
	}
	return false;
}

bool OcclusionCuller::isVoxelVisible(int_t x, int_t y, int_t z, int_t faceData, int_t visibleOnFace)
{
	// Corner points 0-7 and face centres 8-13 of the voxel, chosen by the
	// faces that look towards the viewer. A voxel on the outside of the box
	// on more than one face also samples the corners of its neighbours.
	bool selected[14] = {};
	if (visibleOnFace & ON_MIN_X)
	{
		selected[0] = true;
		if (faceData & ~ON_MIN_X)
			selected[1] = selected[4] = selected[5] = true;
		selected[8] = true;
	}
	if (visibleOnFace & ON_MIN_Y)
	{
		selected[0] = true;
		if (faceData & ~ON_MIN_Y)
			selected[3] = selected[4] = selected[7] = true;
		selected[9] = true;
	}
	if (visibleOnFace & ON_MIN_Z)
	{
		selected[0] = true;
		if (faceData & ~ON_MIN_Z)
			selected[1] = selected[4] = selected[5] = true;
		selected[10] = true;
	}
	if (visibleOnFace & ON_MAX_X)
	{
		selected[4] = true;
		if (faceData & ~ON_MAX_X)
			selected[5] = selected[6] = selected[7] = true;
		selected[11] = true;
	}
	if (visibleOnFace & ON_MAX_Y)
	{
		selected[1] = true;
		if (faceData & ~ON_MAX_Y)
			selected[2] = selected[5] = selected[6] = true;
		selected[12] = true;
	}
	if (visibleOnFace & ON_MAX_Z)
	{
		selected[2] = true;
		if (faceData & ~ON_MAX_Z)
			selected[3] = selected[6] = selected[7] = true;
		selected[13] = true;
	}

	static const double OFFSETS[14][3] = {
		{ 0.05, 0.05, 0.05 }, { 0.05, 0.95, 0.05 }, { 0.05, 0.95, 0.95 }, { 0.05, 0.05, 0.95 },
		{ 0.95, 0.05, 0.05 }, { 0.95, 0.95, 0.05 }, { 0.95, 0.95, 0.95 }, { 0.95, 0.05, 0.95 },
		{ 0.05, 0.5, 0.5 }, { 0.5, 0.05, 0.5 }, { 0.5, 0.5, 0.05 },
		{ 0.95, 0.5, 0.5 }, { 0.5, 0.95, 0.5 }, { 0.5, 0.5, 0.95 },
	};
	int_t count = 0;
	for (int_t i = 0; i < 14; i++)
	{
		if (!selected[i])
			continue;
		targetPoints[count].x = x + OFFSETS[i][0];
		targetPoints[count].y = y + OFFSETS[i][1];
		targetPoints[count].z = z + OFFSETS[i][2];
		count++;
	}
	return isReachable(targetPoints, count);
}

// Whether the segment from the viewer to `target` crosses the block that
// stopped the previous ray. The ray for a neighbouring sample point almost
// always ends in the same wall, so this saves the walk.
bool OcclusionCuller::lineHitsLastBlock(const Point &target)
{
	const double dx = target.x - viewerX;
	const double dy = target.y - viewerY;
	const double dz = target.z - viewerZ;
	const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (length == 0.0)
		return false;

	double tMin = 0.0;
	double tMax = length;
	const double origin[3] = { viewerX, viewerY, viewerZ };
	const double direction[3] = { dx / length, dy / length, dz / length };
	const int_t block[3] = { lastHitX, lastHitY, lastHitZ };
	for (int_t axis = 0; axis < 3; axis++)
	{
		const double lo = block[axis];
		const double hi = block[axis] + 1.0;
		if (direction[axis] == 0.0)
		{
			if (origin[axis] < lo || origin[axis] > hi)
				return false;
			continue;
		}
		double t0 = (lo - origin[axis]) / direction[axis];
		double t1 = (hi - origin[axis]) / direction[axis];
		if (t0 > t1)
		{
			const double swap = t0;
			t0 = t1;
			t1 = swap;
		}
		if (t0 > tMin)
			tMin = t0;
		if (t1 < tMax)
			tMax = t1;
		if (tMin > tMax)
			return false;
	}
	return true;
}

// Walks the grid from the viewer to each target (Amanatides & Woo, as in
// the reference) until a solid block stops it. Returns at the first target
// reached.
bool OcclusionCuller::isReachable(const Point *targets, int_t count)
{
	for (int_t v = 0; v < count; v++)
	{
		const Point &target = targets[v];
		if (allowRayChecks && lineHitsLastBlock(target))
			continue;

		const double dimensionX = std::abs(viewerX - target.x);
		const double dimensionY = std::abs(viewerY - target.y);
		const double dimensionZ = std::abs(viewerZ - target.z);

		// Length of one cell on each axis as a fraction of the ray.
		const double dimFracX = 1.0 / dimensionX;
		const double dimFracY = 1.0 / dimensionY;
		const double dimFracZ = 1.0 / dimensionZ;

		int_t intersectCount = 1;
		int_t xInc, yInc, zInc;
		double tNextX, tNextY, tNextZ;

		if (dimensionX == 0.0)
		{
			xInc = 0;
			tNextX = dimFracX;
		}
		else if (target.x > viewerX)
		{
			xInc = 1;
			intersectCount += Mth::floor(target.x) - cameraX;
			tNextX = (cameraX + 1 - viewerX) * dimFracX;
		}
		else
		{
			xInc = -1;
			intersectCount += cameraX - Mth::floor(target.x);
			tNextX = (viewerX - cameraX) * dimFracX;
		}

		if (dimensionY == 0.0)
		{
			yInc = 0;
			tNextY = dimFracY;
		}
		else if (target.y > viewerY)
		{
			yInc = 1;
			intersectCount += Mth::floor(target.y) - cameraY;
			tNextY = (cameraY + 1 - viewerY) * dimFracY;
		}
		else
		{
			yInc = -1;
			intersectCount += cameraY - Mth::floor(target.y);
			tNextY = (viewerY - cameraY) * dimFracY;
		}

		if (dimensionZ == 0.0)
		{
			zInc = 0;
			tNextZ = dimFracZ;
		}
		else if (target.z > viewerZ)
		{
			zInc = 1;
			intersectCount += Mth::floor(target.z) - cameraZ;
			tNextZ = (cameraZ + 1 - viewerZ) * dimFracZ;
		}
		else
		{
			zInc = -1;
			intersectCount += cameraZ - Mth::floor(target.z);
			tNextZ = (viewerZ - cameraZ) * dimFracZ;
		}

		if (stepRay(cameraX, cameraY, cameraZ, dimFracX, dimFracY, dimFracZ, intersectCount,
			xInc, yInc, zInc, tNextX, tNextY, tNextZ))
		{
			cacheResult(targets[0], true);
			return true;
		}
		allowRayChecks = true;
	}
	cacheResult(targets[0], false);
	return false;
}

bool OcclusionCuller::stepRay(int_t x, int_t y, int_t z, double distInX, double distInY, double distInZ, int_t n,
	int_t xInc, int_t yInc, int_t zInc, double tNextX, double tNextY, double tNextZ)
{
	// A viewer with their head in a wall may look out of it: the ray passes
	// solid blocks until it has been in an open one.
	allowWallClipping = true;

	// The last cell is the target's own and is never tested.
	for (; n > 1; n--)
	{
		const int_t cached = getCacheValue(x, y, z);

		if (cached == CACHE_HIDDEN && !allowWallClipping)
		{
			lastHitX = x;
			lastHitY = y;
			lastHitZ = z;
			return false;
		}

		if (cached == CACHE_UNKNOWN || cached == CACHE_OUT_OF_REACH)
		{
			if (isOpaque(x, y, z))
			{
				if (!allowWallClipping)
				{
					if (lastCacheable)
						setCacheBit(lastEntry, CACHE_HIDDEN << lastOffset);
					lastHitX = x;
					lastHitY = y;
					lastHitZ = z;
					return false;
				}
			}
			else
			{
				allowWallClipping = false;
				if (lastCacheable)
					setCacheBit(lastEntry, CACHE_VISIBLE << lastOffset);
			}
		}

		if (cached == CACHE_VISIBLE)
			allowWallClipping = false;

		if (tNextY < tNextX && tNextY < tNextZ)
		{
			y += yInc;
			tNextY += distInY;
		}
		else if (tNextX < tNextY && tNextX < tNextZ)
		{
			x += xInc;
			tNextX += distInX;
		}
		else
		{
			z += zInc;
			tNextZ += distInZ;
		}
	}
	return true;
}

bool OcclusionCuller::isOpaque(int_t x, int_t y, int_t z)
{
	if (y < 0 || y >= Level::DEPTH)
		return false;
	const int_t cx = x >> 4;
	const int_t cz = z >> 4;
	if (chunk == nullptr || cx != chunkX || cz != chunkZ)
	{
		chunkX = cx;
		chunkZ = cz;
		// An unloaded chunk must not be generated by a ray; it does not occlude.
		chunk = level->hasChunkAt(x, y, z) ? level->getChunk(cx, cz).get() : nullptr;
	}
	if (chunk == nullptr)
		return false;
	// Full cubes that block all light. Tile::solid alone would count leaves,
	// glass and ice, which a mob can be seen through.
	const int_t tile = chunk->getTile(x & 15, y, z & 15);
	return Tile::solid[tile] && Tile::lightBlock[tile] >= 255;
}

int_t OcclusionCuller::getCacheValue(int_t x, int_t y, int_t z)
{
	x -= cameraX;
	y -= cameraY;
	z -= cameraZ;
	const int_t limit = REACH - 2;
	if (std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit)
	{
		lastCacheable = false;
		return CACHE_OUT_OF_REACH;
	}
	const std::size_t side = static_cast<std::size_t>(2 * REACH);
	const std::size_t key = static_cast<std::size_t>(x + REACH) +
		static_cast<std::size_t>(y + REACH) * side + static_cast<std::size_t>(z + REACH) * side * side;
	lastEntry = key / 4;
	lastOffset = static_cast<int_t>(key % 4) * 2;
	lastCacheable = true;
	return (cache[lastEntry] >> lastOffset) & 3;
}

void OcclusionCuller::setCacheBit(std::size_t entry, int_t bit)
{
	if (cache[entry] == 0)
		touched.push_back(static_cast<std::uint32_t>(entry));
	cache[entry] |= static_cast<std::uint8_t>(bit);
}

void OcclusionCuller::cacheResult(const Point &point, bool visible)
{
	const int_t value = getCacheValue(Mth::floor(point.x), Mth::floor(point.y), Mth::floor(point.z));
	if (value == CACHE_OUT_OF_REACH)
		return;
	setCacheBit(lastEntry, (visible ? CACHE_VISIBLE : CACHE_HIDDEN) << lastOffset);
}

void OcclusionCuller::resetCache()
{
	for (std::uint32_t entry : touched)
		cache[entry] = 0;
	touched.clear();
}
