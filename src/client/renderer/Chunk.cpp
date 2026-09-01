#include "client/renderer/Chunk.h"

#include <algorithm>
#include <unordered_set>

#include "client/renderer/Tesselator.h"
#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/TileRenderer.h"
#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "client/renderer/tileentity/SignRenderer.h"

#include "world/level/Region.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/tile/entity/SignTileEntity.h"

#include "util/Mth.h"
#include "legacygl/Context.h"

int_t Chunk::updates = 0;

Tesselator &Chunk::t = Tesselator::instance;

Chunk::Chunk(Level &level, std::vector<std::shared_ptr<TileEntity>> &globalRenderableTileEntities, int_t x, int_t y, int_t z, int_t size, int_t lists) : level(level), globalRenderableTileEntities(globalRenderableTileEntities)
{
	xs = ys = zs = size;
	radius = Mth::sqrt(static_cast<float>(xs * xs + ys * ys + zs * zs)) / 2.0f;
	this->lists = lists;

	this->x = -999;
	setPos(x, y, z);

	dirty = false;
}

void Chunk::setPos(int_t x, int_t y, int_t z)
{
	if (this->x == x && this->y == y && this->z == z) return;

	// The slot is moving: the old position's terrain lists will never be
	// called again, but nothing recompiles them until the new position's
	// rebuild. Retire their retained payload now, or every wrap of the
	// renderer grid strands the previous geometry behind `empty[layer]`.
	legacygl::context().retireDisplayListPayload(static_cast<unsigned int>(lists));
	legacygl::context().retireDisplayListPayload(static_cast<unsigned int>(lists + 1));

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

	glNewList(lists + 2, GL_COMPILE);
	EntityRenderer::renderFlat(*bb);
	glEndList();

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

	std::vector<std::shared_ptr<TileEntity>> discoveredTileEntities;

	int_t r = 1;
	Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
	TileRenderer tileRenderer(&region);

	for (int_t i = 0; i < 2; i++)
	{
		bool renderNextLayer = false;
		bool rendered = false;

		bool started = false;

		for (int_t y = y0; y < y1; y++)
		{
			for (int_t z = z0; z < z1; z++)
			{
				for (int_t x = x0; x < x1; x++)
				{
					int_t tileId = region.getTile(x, y, z);
					if (tileId > 0)
					{
						if (!started)
						{
							started = true;

							glNewList(lists + i, GL_COMPILE);
							
							glPushMatrix();
							translateToPos();

							float ss = 1.0000001f;
							glTranslatef(-zs / 2.0f, -ys / 2.0f, -zs / 2.0f);
							glScalef(ss, ss, ss);
							glTranslatef(zs / 2.0f, ys / 2.0f, zs / 2.0f);

							t.begin();
							t.offset(-this->x, -this->y, -this->z);
						}

					if (i == 0 && Tile::isEntityTile[tileId])
					{
						std::shared_ptr<TileEntity> tileEntity = region.getTileEntity(x, y, z);
						if (tileEntity != nullptr && TileEntityRenderDispatcher::instance.hasRenderer(tileEntity.get()))
							discoveredTileEntities.push_back(tileEntity);
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
		}

		if (started)
		{
			t.end();
			glPopMatrix();
			glEndList();
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
	// A rebuild that proves a layer empty leaves the old compiled list
	// untouched (the draw path checks `empty[layer]`), so drop its payload.
	// This covers a section that became air and a section that no longer
	// needs the translucent layer.
	for (int_t i = 0; i < 2; i++)
	{
		if (empty[i])
			legacygl::context().retireDisplayListPayload(
				static_cast<unsigned int>(lists + i));
	}
	compiled = true;
	reconcileRenderableTileEntities(renderableTileEntities,
		globalRenderableTileEntities, discoveredTileEntities);
}


void Chunk::reconcileRenderableTileEntities(
	std::vector<std::shared_ptr<TileEntity>> &current,
	std::vector<std::shared_ptr<TileEntity>> &global,
	const std::vector<std::shared_ptr<TileEntity>> &discovered)
{
	// Alpha takes a set snapshot of the old local list, builds a new local
	// list, adds new-old to the global list, then removes old-new from it
	// (WorldRenderer.java:114-116,169-174).
	std::unordered_set<std::shared_ptr<TileEntity>> oldSet(current.begin(), current.end());
	std::unordered_set<std::shared_ptr<TileEntity>> newSet(discovered.begin(), discovered.end());

	current = discovered;

	for (const std::shared_ptr<TileEntity> &tileEntity : newSet)
	{
		if (oldSet.find(tileEntity) == oldSet.end())
			global.push_back(tileEntity);
	}

	for (const std::shared_ptr<TileEntity> &tileEntity : oldSet)
	{
		if (newSet.find(tileEntity) != newSet.end())
			continue;
		global.erase(std::remove(global.begin(), global.end(), tileEntity), global.end());

		// A renderer chunk rebuild/unload is the lifetime boundary for cached
		// sign command streams. Remove the GL list while the shared_ptr is still
		// valid so exploration cannot accumulate stale display lists.
		SignTileEntity *sign = dynamic_cast<SignTileEntity *>(tileEntity.get());
		if (sign != nullptr)
		{
			SignRenderer *signRenderer = dynamic_cast<SignRenderer *>(
				TileEntityRenderDispatcher::instance.getRenderer(sign));
			if (signRenderer != nullptr)
				signRenderer->invalidateWorldSign(sign);
		}
	}
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
}

void Chunk::remove()
{
	std::vector<std::shared_ptr<TileEntity>> none;
	reconcileRenderableTileEntities(renderableTileEntities,
		globalRenderableTileEntities, none);
	reset();
	// The renderer chunk is going away (world unload or grid resize); its
	// terrain and bounding-box payloads have no further caller.
	for (int_t i = 0; i < 3; i++)
		legacygl::context().retireDisplayListPayload(
			static_cast<unsigned int>(lists + i));
}
int_t Chunk::getList(int_t layer)
{
	if (!visible) return -1;
	if (!empty[layer]) return lists + layer;
	return -1;
}
int_t Chunk::getAllLists(std::vector<int_t> displayLists, int_t p, int_t layer)
{
	if (!visible) return p;
	if (!empty[layer]) displayLists[p++] = lists + layer;
	return p;
}

void Chunk::cull(Culler &culler)
{
	visible = culler.isVisible(*bb);
}

void Chunk::renderBB()
{
	glCallList(lists + 2);
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
