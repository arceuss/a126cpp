#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "client/renderer/tileentity/TileEntityRenderer.h"
#include "client/model/SignModel.h"
#include "OpenGL.h"

class SignTileEntity;
class Culler;

// newb12: TileEntitySignRenderer (TileEntitySignRenderer.java)
class SignRenderer : public TileEntityRenderer
{
private:
	// World signs are batched per region of REGION_SIZE blocks on each axis.
	// One display list per region holds every queued sign's board and text
	// geometry, pre-transformed into the region's own frame, so a region draws
	// as two draws (boards, then text) however many signs it holds.
	static const int_t REGION_SHIFT = 5;
	static const int_t REGION_SIZE = 1 << REGION_SHIFT;

	struct RegionKey
	{
		int_t x = 0;
		int_t y = 0;
		int_t z = 0;

		bool operator==(const RegionKey &other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct RegionKeyHash
	{
		std::size_t operator()(const RegionKey &key) const;
	};

	// What a compiled region was built from. A frame whose queued signs match
	// this exactly reuses the list; anything else recompiles the region.
	struct QueuedSign
	{
		SignTileEntity *owner = nullptr;
		int_t x = 0;
		int_t y = 0;
		int_t z = 0;
		float brightness = 0.0f;
		int_t tileId = 0;
		int_t data = 0;

		bool operator==(const QueuedSign &other) const
		{
			return owner == other.owner && x == other.x && y == other.y && z == other.z &&
				brightness == other.brightness && tileId == other.tileId && data == other.data;
		}
	};

	struct CachedRegion
	{
		GLuint list = 0;
		std::vector<QueuedSign> signs;
	};

	SignModel signModel;
	std::unordered_map<RegionKey, CachedRegion, RegionKeyHash> regionCache;
	std::unordered_map<RegionKey, std::vector<QueuedSign>, RegionKeyHash> pendingRegions;

	void renderImpl(SignTileEntity &sign, double x, double y, double z, float a,
		bool omitPositionTranslate);
	GLuint compileRegionList(const RegionKey &key, const std::vector<QueuedSign> &signs);
	static void deleteRegionList(CachedRegion &cached);
	static RegionKey regionKeyFor(int_t x, int_t y, int_t z);

public:
	SignRenderer();
	
	// newb12: void render(SignTileEntity sign, double x, double y, double z, float a) (SignRenderer.java:12)
	void render(SignTileEntity &sign, double x, double y, double z, float a);
	
	// Helper to render from TileEntity*
	void renderEntity(TileEntity *entity, double x, double y, double z, float a);

	// World-only cache. Signs that pass LevelRenderer's per-sign distance and
	// frustum tests are queued; flushWorldBatch draws each region that has
	// queued signs with Alpha's exact camera-relative translate, compiling the
	// region first when its sign set or any sign's brightness changed. The
	// edit-screen path continues to use render() directly.
	bool queueWorld(SignTileEntity &sign, float brightness);
	void flushWorldBatch(Culler &culler);
	void invalidateWorldSign(SignTileEntity *sign);
	// Developer fixture control: false makes queueWorld decline every sign so
	// LevelRenderer falls through to the per-sign render() path, which lets a
	// bench compare the batched image against Alpha's original chain.
	bool batchWorldSigns = true;
	void invalidateWorldSignAt(int_t x, int_t y, int_t z);
	void clearWorldCache();
};
