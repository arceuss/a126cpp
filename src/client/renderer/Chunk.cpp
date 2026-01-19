#include "client/renderer/Chunk.h"

#include "client/renderer/Tesselator.h"
#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/TileRenderer.h"
#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"

#include "world/level/Region.h"
#include "world/level/chunk/LevelChunk.h"

#include "util/Mth.h"

int_t Chunk::updates = 0;

Tesselator &Chunk::t = Tesselator::instance;

Chunk::Chunk(Level &level, std::vector<std::shared_ptr<TileEntity>> &globalRenderableTileEntities, int_t x, int_t y, int_t z, int_t size) : level(level), globalRenderableTileEntities(globalRenderableTileEntities)
{
	xs = ys = zs = size;
	radius = Mth::sqrt(static_cast<float>(xs * xs + ys * ys + zs * zs)) / 2.0f;
	
	// Initialize VBO storage
	layerVBOs[0] = ChunkLayerVBO();
	layerVBOs[1] = ChunkLayerVBO();

	this->x = -999;
	setPos(x, y, z);

	dirty = false;
}

Chunk::~Chunk()
{
	// Cleanup VBOs
	layerVBOs[0].clear();
	layerVBOs[1].clear();
}

void Chunk::setPos(int_t x, int_t y, int_t z)
{
	if (this->x == x && this->y == y && this->z == z) return;

	reset();
	this->x = x;
	this->y = y;
	this->z = z;
	xm = x + xs / 2;
	ym = y + ys / 2;
	zm = z + zs / 2;

	xRenderOffs = x & 0x3FF;
	yRenderOffs = y;
	zRenderOffs = z & 0x3FF;
	xRender = x - xRenderOffs;
	yRender = y - yRenderOffs;
	zRender = z - zRenderOffs;

	float g = 6.0f;
	bb.reset(AABB::newPermanent(x - g, y - g, z - g, x + xs + g, y + ys + g, z + zs + g));

	setDirty();
}

void Chunk::translateToPos()
{
	glTranslatef(xRenderOffs, yRenderOffs, zRenderOffs);
}

void Chunk::rebuild()
{
	if (!dirty) return;
	updates++;

	int_t x0 = x;
	int_t y0 = y;
	int_t z0 = z;
	int_t x1 = x + xs;
	int_t y1 = y + ys;
	int_t z1 = z + zs;

	empty.fill(true);

	LevelChunk::touchedSky = false;

	std::unordered_set<std::shared_ptr<TileEntity>> oldTileEntities;
	oldTileEntities.insert(renderableTileEntities.begin(), renderableTileEntities.end());
	renderableTileEntities.clear();

	int_t r = 1;
	Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
	TileRenderer tileRenderer(&region);

		for (int_t i = 0; i < 2; i++)
	{
		bool renderNextLayer = false;
		bool rendered = false;

		bool started = false;

		// Ensure tesselator is in a clean state before starting this layer
		// If somehow it's still tesselating, reset it (safety check)
		t.reset();

		// Optimized: Loop order for better cache locality (y-z-x is better for Minecraft's chunk layout)
		for (int_t y = y0; y < y1; y++)
		{
			for (int_t z = z0; z < z1; z++)
			{
				for (int_t x = x0; x < x1; x++)
				{
					int_t tileId = region.getTile(x, y, z);
					// Early skip for air (optimization: most blocks are air)
					if (tileId <= 0)
						continue;
					
					if (!started)
					{
						started = true;

						// Clear old VBO for this layer before rebuilding
						layerVBOs[i].clear();

						// Ensure tesselator is clean before starting
						t.reset();
						t.begin();
						// Apply chunk offset to convert world coords to chunk-local coords
						// The render offset and scaling will be applied at render time
						t.offset(-this->x, -this->y, -this->z);
					}

					if (i == 0 && Tile::isEntityTile[tileId])
					{
						// newb12: Collect tile entities for rendering (Chunk.java:148-152)
						// newb12: TileEntity et = region.getTileEntity(x, y, z);
						// newb12: if (TileEntityRenderDispatcher.instance.hasRenderer(et)) {
						// newb12:     this.renderableTileEntities.add(et);
						// newb12: }
						std::shared_ptr<TileEntity> tileEntity = region.getTileEntity(x, y, z);
						if (tileEntity != nullptr && TileEntityRenderDispatcher::instance.hasRenderer(tileEntity.get()))
						{
							globalRenderableTileEntities.push_back(tileEntity);
						}
					}

					Tile *tile = Tile::tiles[tileId];
					int_t renderLayer = tile->getRenderLayer();
					if (renderLayer != i)
					{
						renderNextLayer = true;
					}
					else if (renderLayer == i)
					{
						rendered |= tileRenderer.tesselateInWorld(*tile, x, y, z);
					}
				}
			}
		}

		if (started)
		{
			// Build VBO from tesselator data instead of ending display list
			int_t vertexCount = 0;
			bool hasTex = false;
			bool hasCol = false;
			bool hasNorm = false;
			GLuint vboId = t.buildVBO(vertexCount, hasTex, hasCol, hasNorm);
			
			if (vboId != 0 && vertexCount > 0) {
				layerVBOs[i].vboId = vboId;
				layerVBOs[i].vertexCount = vertexCount;
				layerVBOs[i].hasTexture = hasTex;
				layerVBOs[i].hasColor = hasCol;
				layerVBOs[i].hasNormal = hasNorm;
				layerVBOs[i].valid = true;
			} else {
				layerVBOs[i].valid = false;
			}
			
			t.clear();
			t.offset(0.0, 0.0, 0.0);
		}
		else
		{
			rendered = false;
		}

		if (rendered) empty[i] = false;
		if (!renderNextLayer) break;
	}

	skyLit = LevelChunk::touchedSky;
	compiled = true;
}

float Chunk::distanceToSqr(Entity &player)
{
	float dx = static_cast<float>(player.x - static_cast<double>(xm));
	float dy = static_cast<float>(player.y - static_cast<double>(ym));
	float dz = static_cast<float>(player.z - static_cast<double>(zm));
	return dx * dx + dy * dy + dz * dz;
}

float Chunk::squishedDistanceToSqr(Entity &player)
{
	float dx = static_cast<float>(player.x - static_cast<double>(xm));
	float dy = static_cast<float>(player.y - static_cast<double>(ym)) * 2.0f;
	float dz = static_cast<float>(player.z - static_cast<double>(zm));
	return dx * dx + dy * dy + dz * dz;
}

void Chunk::reset()
{
	empty.fill(true);
	visible = false;
	compiled = false;
	// Don't clear VBOs here - they're still valid until rebuild()
}

void Chunk::remove()
{
	reset();
}

bool Chunk::getVBO(int_t layer, GLuint &vboId, int_t &vertexCount, bool &hasTex, bool &hasCol, bool &hasNorm) const
{
	if (!visible || layer < 0 || layer >= 2) {
		return false;
	}
	if (empty[layer] || !layerVBOs[layer].valid) {
		return false;
	}
	vboId = layerVBOs[layer].vboId;
	vertexCount = layerVBOs[layer].vertexCount;
	hasTex = layerVBOs[layer].hasTexture;
	hasCol = layerVBOs[layer].hasColor;
	hasNorm = layerVBOs[layer].hasNormal;
	return true;
}

void Chunk::cull(Culler &culler)
{
	// Performance optimization: Early height-based culling for underground chunks
	// If chunk is far below player's view, skip expensive frustum test
	// This is much cheaper than frustum test and catches most underground chunks
	constexpr double UNDERGROUND_CULL_HEIGHT = 32.0;  // Skip chunks 32+ blocks below player view
	
	// Check if chunk is significantly below the viewing frustum
	// Use bottom of chunk bounding box vs player's approximate view height
	// This catches things underground that frustum might not optimize for
	if (bb->y1 < culler.getY() - UNDERGROUND_CULL_HEIGHT)
	{
		visible = false;
		return;
	}
	
	// Full frustum culling test for chunks that might be visible
	visible = culler.isVisible(*bb);
}

void Chunk::renderBB()
{
	// Use EntityRenderer for bounding box rendering (debug rendering only)
	if (bb != nullptr) {
		EntityRenderer::renderFlat(*bb);
	}
}

bool Chunk::isEmpty()
{
	if (!compiled) return false;
	return empty[0] && empty[1];
}

void Chunk::setDirty()
{
	dirty = true;
}
