#include "client/renderer/Chunk.h"

#include <algorithm>
#include <unordered_set>

#include "client/renderer/Tesselator.h"
#include "client/renderer/ChunkSnapshot.h"
#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/TileRenderer.h"
#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"

#include "world/level/Region.h"
#include "world/level/chunk/LevelChunk.h"

#include "util/Mth.h"

#include "OpenGL.h"

std::atomic<int_t> Chunk::updates = 0;

Chunk::Chunk(Level &level, std::vector<std::shared_ptr<TileEntity>> &globalRenderableTileEntities, int_t x, int_t y, int_t z, int_t size, int_t lists) : level(level), globalRenderableTileEntities(globalRenderableTileEntities)
{
	xs = ys = zs = size;
	radius = Mth::sqrt(static_cast<float>(xs * xs + ys * ys + zs * zs)) / 2.0f;
	this->lists = lists;

	this->x = -999;
	setPos(x, y, z);

	dirty = false;
}

Chunk::~Chunk()
{
	deleteVBOs();
}

void Chunk::invalidateVBOs()
{
	for (int_t i = 0; i < 2; i++)
	{
		vboEntries[i].vertexCount = 0;
		vboEntries[i].empty = true;
	}
	useVBO = false;
}

void Chunk::deleteVBOs()
{
	for (int_t i = 0; i < 2; i++)
	{
		if (vboEntries[i].vboId != 0)
		{
			glDeleteBuffers(1, &vboEntries[i].vboId);
			vboEntries[i].vboId = 0;
		}
		vboEntries[i].vertexCount = 0;
		vboEntries[i].empty = true;
	}
	useVBO = false;
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

	glNewList(lists + 2, GL_COMPILE);
	EntityRenderer::renderFlat(*bb);
	glEndList();

	setDirty();
}

void Chunk::translateToPos()
{
	glTranslatef(xRenderOffs, yRenderOffs, zRenderOffs);
}

void Chunk::removeRenderableTileEntitiesFromGlobal()
{
	if (renderableTileEntities.empty())
	{
		return;
	}

	std::unordered_set<TileEntity *> toRemove;
	for (const auto &te : renderableTileEntities)
		toRemove.insert(te.get());

	globalRenderableTileEntities.erase(std::remove_if(globalRenderableTileEntities.begin(), globalRenderableTileEntities.end(), [&toRemove](const std::shared_ptr<TileEntity> &tileEntity)
	{
		return toRemove.count(tileEntity.get()) != 0;
	}), globalRenderableTileEntities.end());

	renderableTileEntities.clear();
}

void Chunk::addRenderableTileEntitiesToGlobal()
{
	if (renderableTileEntities.empty())
		return;

	std::unordered_set<TileEntity *> existing;
	for (const auto &te : globalRenderableTileEntities)
		existing.insert(te.get());

	for (const auto &tileEntity : renderableTileEntities)
	{
		if (existing.count(tileEntity.get()) == 0)
		{
			globalRenderableTileEntities.push_back(tileEntity);
		}
	}
}

// Original synchronous rebuild - uses display lists (legacy path)
void Chunk::rebuild()
{
	if (!dirty) return;
	updates++;
	deleteVBOs();

	// Get the thread-local Tesselator (LCE _LARGE_WORLDS pattern)
	Tesselator &t = Tesselator::getInstance();

	int_t x0 = x;
	int_t y0 = y;
	int_t z0 = z;
	int_t x1 = x + xs;
	int_t y1 = y + ys;
	int_t z1 = z + zs;

	empty.fill(true);

	LevelChunk::touchedSky = false;

	removeRenderableTileEntitiesFromGlobal();

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
						{
							renderableTileEntities.push_back(tileEntity);
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

	addRenderableTileEntitiesToGlobal();

	skyLit = LevelChunk::touchedSky;
	compiled = true;
	useVBO = false;
}

// Off-thread mesh building: generates vertex data into ChunkBuildResult, NO GL calls
void Chunk::buildMesh(ChunkSnapshot &snapshot, Tesselator &localTess, ChunkBuildResult &result)
{
	updates++;

	result.touchedSky = false;
	snapshot.touchedSky = false;

	TileRenderer tileRenderer(&snapshot);

	int_t x0 = x;
	int_t y0 = y;
	int_t z0 = z;
	int_t x1 = x + xs;
	int_t y1 = y + ys;
	int_t z1 = z + zs;

	for (int_t i = 0; i < 2; i++)
	{
		bool renderNextLayer = false;
		bool rendered = false;
		bool started = false;

		result.layers[i] = ChunkMeshData();

		for (int_t y = y0; y < y1; y++)
		{
			for (int_t z = z0; z < z1; z++)
			{
				for (int_t x = x0; x < x1; x++)
				{
					int_t tileId = snapshot.getTile(x, y, z);
					if (tileId > 0)
					{
						if (!started)
						{
							started = true;
							localTess.setOutputTarget(&result.layers[i]);
							localTess.begin();
							localTess.offset(
								static_cast<double>(xRenderOffs) - static_cast<double>(this->x),
								static_cast<double>(yRenderOffs) - static_cast<double>(this->y),
								static_cast<double>(zRenderOffs) - static_cast<double>(this->z));
						}

						// Collect tile entities (layer 0 only)
						if (i == 0 && Tile::isEntityTile[tileId])
						{
							std::shared_ptr<TileEntity> tileEntity = snapshot.getTileEntity(x, y, z);
							if (tileEntity != nullptr && TileEntityRenderDispatcher::instance.hasRenderer(tileEntity.get()))
							{
								result.foundTileEntities.push_back(tileEntity);
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
		}

		if (started)
		{
			localTess.end();
			localTess.offset(0.0, 0.0, 0.0);
			localTess.setOutputTarget(nullptr);
		}
		else
		{
			rendered = false;
		}

		if (rendered) result.layers[i].empty = false;
		if (!renderNextLayer) break;
	}

	result.touchedSky = snapshot.touchedSky;
}

// Main-thread VBO upload from completed build result
void Chunk::uploadMesh(ChunkBuildResult &result)
{
	removeRenderableTileEntitiesFromGlobal();

	for (int_t i = 0; i < 2; i++)
	{
		ChunkMeshData &mesh = result.layers[i];
		empty[i] = mesh.empty;

		if (!mesh.empty && mesh.vertexCount > 0)
		{
			if (vboEntries[i].vboId == 0)
				glGenBuffers(1, &vboEntries[i].vboId);

			glBindBuffer(GL_ARRAY_BUFFER, vboEntries[i].vboId);
			glBufferData(GL_ARRAY_BUFFER, mesh.dataSize, mesh.vertexData.get(), GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			vboEntries[i].vertexCount = mesh.vertexCount;
			vboEntries[i].hasTexture = mesh.hasTexture;
			vboEntries[i].hasColor = mesh.hasColor;
			vboEntries[i].hasNormal = mesh.hasNormal;
			vboEntries[i].empty = false;
		}
		else
		{
			if (vboEntries[i].vboId != 0)
			{
				glDeleteBuffers(1, &vboEntries[i].vboId);
				vboEntries[i].vboId = 0;
			}
			vboEntries[i].vertexCount = 0;
			vboEntries[i].empty = true;
		}
	}

	// Apply tile entities
	renderableTileEntities = std::move(result.foundTileEntities);
	addRenderableTileEntitiesToGlobal();

	skyLit = result.touchedSky;
	compiled = true;
	useVBO = true;
	dirty = false;
	inFlight = false;
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
	removeRenderableTileEntitiesFromGlobal();
	invalidateVBOs();
	empty.fill(true);
	visible = false;
	compiled = false;
	inFlight = false;
}

void Chunk::remove()
{
	reset();
	deleteVBOs();
}

int_t Chunk::getList(int_t layer)
{
	if (useVBO) return -1; // VBO path doesn't use display lists
	if (!visible) return -1;
	if (!empty[layer]) return lists + layer;
	return -1;
}

int_t Chunk::getAllLists(std::vector<int_t> displayLists, int_t p, int_t layer)
{
	if (useVBO) return p;
	if (!visible) return p;
	if (!empty[layer]) displayLists[p++] = lists + layer;
	return p;
}

const ChunkVBOEntry *Chunk::getVBOEntry(int_t layer)
{
	if (!visible) return nullptr;
	if (vboEntries[layer].empty) return nullptr;
	return &vboEntries[layer];
}

void Chunk::renderVBO(int_t layer)
{
	const ChunkVBOEntry *entry = getVBOEntry(layer);
	if (entry == nullptr || entry->vboId == 0) return;

	glBindBuffer(GL_ARRAY_BUFFER, entry->vboId);

	glVertexPointer(3, GL_FLOAT, 32, reinterpret_cast<void *>(0));
	glEnableClientState(GL_VERTEX_ARRAY);

	if (entry->hasTexture)
	{
		glTexCoordPointer(2, GL_FLOAT, 32, reinterpret_cast<void *>(12));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (entry->hasColor)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, 32, reinterpret_cast<void *>(20));
		glEnableClientState(GL_COLOR_ARRAY);
	}

	if (entry->hasNormal)
	{
		glNormalPointer(GL_BYTE, 32, reinterpret_cast<void *>(24));
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	glDrawArrays(GL_TRIANGLES, 0, entry->vertexCount);

	glDisableClientState(GL_VERTEX_ARRAY);
	if (entry->hasTexture)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (entry->hasColor)
		glDisableClientState(GL_COLOR_ARRAY);
	if (entry->hasNormal)
		glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
