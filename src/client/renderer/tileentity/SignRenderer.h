#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

#include "client/renderer/tileentity/TileEntityRenderer.h"
#include "client/model/SignModel.h"
#include "OpenGL.h"

class SignTileEntity;

// newb12: TileEntitySignRenderer (TileEntitySignRenderer.java)
class SignRenderer : public TileEntityRenderer
{
private:
	struct WorldSignKey
	{
		int_t x = 0;
		int_t y = 0;
		int_t z = 0;

		bool operator==(const WorldSignKey &other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct WorldSignKeyHash
	{
		std::size_t operator()(const WorldSignKey &key) const;
	};

	struct CachedWorldSign
	{
		GLuint list = 0;
		float brightness = -1.0f;
		int_t tileId = -1;
		int_t data = -1;
		SignTileEntity *owner = nullptr;
	};

	// A queued sign keeps its block position so the submission loop can
	// reproduce Alpha's camera-relative translate in double precision.
	struct QueuedWorldSign
	{
		GLuint list = 0;
		int_t x = 0;
		int_t y = 0;
		int_t z = 0;
	};

	SignModel signModel;
	std::unordered_map<WorldSignKey, CachedWorldSign, WorldSignKeyHash> worldCache;
	std::vector<QueuedWorldSign> worldBatch;

	void renderImpl(SignTileEntity &sign, double x, double y, double z, float a,
		bool immediateGeometry, bool omitPositionTranslate);
	GLuint compileWorldList(SignTileEntity &sign, float brightness, int_t tileId, int_t data);
	void deleteCachedList(CachedWorldSign &cached);
	static WorldSignKey makeWorldSignKey(int_t x, int_t y, int_t z);

public:
	SignRenderer();
	
	// newb12: void render(SignTileEntity sign, double x, double y, double z, float a) (SignRenderer.java:12)
	void render(SignTileEntity &sign, double x, double y, double z, float a);
	
	// Helper to render from TileEntity*
	void renderEntity(TileEntity *entity, double x, double y, double z, float a);

	// World-only cache. Each immutable sign is flattened into one server-side
	// OpenGL display list holding its rotation, board and per-glyph text
	// geometry. The sign's world position stays out of that list, because a
	// large absolute coordinate rounded to float loses its low bits and makes
	// distant signs jitter; flushWorldBatch applies Alpha's exact
	// camera-relative translate instead. LevelRenderer preserves Alpha's
	// per-sign distance/frustum decisions and list order, and the edit-screen
	// path continues to use render() directly.
	bool queueWorld(SignTileEntity &sign, float brightness);
	void flushWorldBatch();
	void invalidateWorldSign(SignTileEntity *sign);
	void invalidateWorldSignAt(int_t x, int_t y, int_t z);
	void clearWorldCache();
};
