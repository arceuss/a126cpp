#pragma once

#include <algorithm>
#include <array>
#include <atomic>

#include "client/renderer/Tesselator.h"
#include "client/renderer/culling/Culler.h"

#include "world/level/Level.h"
#include "world/level/tile/entity/TileEntity.h"

#include "java/Type.h"

class ChunkSnapshot;

// Render data for one layer of a chunk (opaque or translucent)
struct ChunkVBOEntry
{
	unsigned int vboId = 0;
	int_t vertexCount = 0;
	bool hasTexture = false;
	bool hasColor = false;
	bool hasNormal = false;
	bool empty = true;
};

// Result of an off-thread buildMesh() call
struct ChunkBuildResult
{
	std::array<ChunkMeshData, 2> layers;
	std::vector<std::shared_ptr<TileEntity>> foundTileEntities;
	bool touchedSky = false;
};

class Chunk
{
public:
	Level &level;

private:
	int_t lists = -1;

	// VBO rendering data (replaces display lists for chunk geometry)
	std::array<ChunkVBOEntry, 2> vboEntries = {};
	bool useVBO = false;

public:
	int_t x = 0, y = 0, z = 0;
	int_t xs = 0, ys = 0, zs = 0;

	static std::atomic<int_t> updates;

	int_t xRender = 0, yRender = 0, zRender = 0;
	int_t xRenderOffs = 0, yRenderOffs = 0, zRenderOffs = 0;

	bool visible = false;
	std::array<bool, 2> empty = {};

	int_t xm = 0, ym = 0, zm = 0;

	float radius = 0.0f;
	bool dirty = false;

	std::unique_ptr<AABB> bb;

	int_t id = 0;

	bool occlusion_visible = false;
	bool occlusion_querying = false;
	int_t occlusion_id = 0;

	bool skyLit = false;

	// True when a worker thread is currently building this chunk's mesh
	bool inFlight = false;

private:
	bool compiled = false;

public:
	std::vector<std::shared_ptr<TileEntity>> renderableTileEntities;

private:
	std::vector<std::shared_ptr<TileEntity>> &globalRenderableTileEntities;

public:
	Chunk(Level &level, std::vector<std::shared_ptr<TileEntity>> &globalRenderableTileEntities, int_t x, int_t y, int_t z, int_t size, int_t lists);
	~Chunk();

	void setPos(int_t x, int_t y, int_t z);

private:
	void translateToPos();
	void removeRenderableTileEntitiesFromGlobal();
	void addRenderableTileEntitiesToGlobal();

public:
	// Original synchronous rebuild (still used for force mode / initial load)
	void rebuild();

	// Async mesh building: buildMesh runs off-thread, uploadMesh on main thread
	void buildMesh(ChunkSnapshot &snapshot, Tesselator &localTess, ChunkBuildResult &result);
	void uploadMesh(ChunkBuildResult &result);

	float distanceToSqr(Entity &player);
	float squishedDistanceToSqr(Entity &player);

	void reset();
	void remove();

	// Display-list based (legacy, for bounding box only)
	int_t getList(int_t layer);
	int_t getAllLists(std::vector<int_t> displayLists, int_t p, int_t layer);

	// VBO-based rendering
	const ChunkVBOEntry *getVBOEntry(int_t layer);
	void renderVBO(int_t layer);

	void cull(Culler &culler);

	void renderBB();

	bool isEmpty();

	void setDirty();

private:
	void deleteVBOs();
	void invalidateVBOs();
};
