#pragma once

#include <cstdint>
#include <vector>

#include "client/renderer/culling/OcclusionCullState.h"
#include "java/Type.h"

class AABB;
class Level;
class LevelChunk;

// Occlusion test for entity rendering: an entity whose box cannot be reached
// from the viewer's eye by a straight line through non-solid blocks is not
// drawn. In Alpha most mobs a daytime player is near are in caves under the
// player, and every one inside the frustum was being drawn against the
// terrain.
//
// The ray walk, the sample points on the box faces and the per-viewer voxel
// cache follow the OcclusionCulling library by LogisticsCraft (MIT), which
// tr7zw's EntityCulling drives from a thread. This port runs on the render
// thread: entities are tested at most once per level tick and the cache is
// reset when the tick or the viewer's block changes.
class OcclusionCuller
{
public:
	// Blocks from the viewer within which rays are traced. Entities beyond
	// this are drawn.
	static const int_t REACH = 128;

	// Developer fixture control: false draws every entity the frustum admits.
	static bool enabled;

	OcclusionCuller();

	// Starts the pass for a frame. `x, y, z` is the viewer's eye.
	void beginPass(Level &level, double x, double y, double z, int_t tick);

	// Whether an entity with the given box and memory should be drawn. Tests
	// the box at most once per pass; a visible result holds for two ticks so
	// an entity at a wall's edge does not flicker with the viewer's steps.
	bool shouldRender(OcclusionCullState &state, const AABB &box);

	// Whether a straight line reaches any sample point on the box's faces.
	bool isVisible(const AABB &box);

private:
	// Face bits of a voxel of the box.
	static const int_t ON_MIN_X = 0x01;
	static const int_t ON_MAX_X = 0x02;
	static const int_t ON_MIN_Y = 0x04;
	static const int_t ON_MAX_Y = 0x08;
	static const int_t ON_MIN_Z = 0x10;
	static const int_t ON_MAX_Z = 0x20;

	// Cache states per voxel, relative to the viewer's block.
	static const int_t CACHE_UNKNOWN = 0;
	static const int_t CACHE_VISIBLE = 1;
	static const int_t CACHE_HIDDEN = 2;
	static const int_t CACHE_OUT_OF_REACH = -1;

	struct Point
	{
		double x, y, z;
	};

	bool isVoxelVisible(int_t x, int_t y, int_t z, int_t faceData, int_t visibleOnFace);
	bool isReachable(const Point *targets, int_t count);
	bool stepRay(int_t x, int_t y, int_t z, double distInX, double distInY, double distInZ, int_t n,
		int_t xInc, int_t yInc, int_t zInc, double tNextX, double tNextY, double tNextZ);
	bool lineHitsLastBlock(const Point &target);
	bool isOpaque(int_t x, int_t y, int_t z);

	int_t getCacheValue(int_t x, int_t y, int_t z);
	void setCacheBit(std::size_t entry, int_t bit);
	void cacheResult(const Point &point, bool visible);
	void resetCache();

	Level *level = nullptr;
	int_t passId = 0;
	int_t passTick = -1;
	double viewerX = 0.0, viewerY = 0.0, viewerZ = 0.0;
	int_t cameraX = 0, cameraY = 0, cameraZ = 0;

	// Chunk the current ray is walking through, held for one isVisible call.
	LevelChunk *chunk = nullptr;
	int_t chunkX = 0, chunkZ = 0;

	// Two bits per voxel in a cube of side 2 * REACH around the viewer. The
	// entries a pass touched are cleared at the next reset instead of the
	// whole array.
	std::vector<std::uint8_t> cache;
	std::vector<std::uint32_t> touched;
	std::size_t lastEntry = 0;
	int_t lastOffset = 0;
	bool lastCacheable = false;

	std::vector<std::uint8_t> skipList;
	Point targetPoints[14] = {};
	bool allowRayChecks = false;
	bool allowWallClipping = false;
	int_t lastHitX = 0, lastHitY = 0, lastHitZ = 0;
};
