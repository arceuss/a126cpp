#include "world/level/chunk/LevelChunk.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/TilePos.h"
#include "util/Mth.h"
#include <iostream>

bool LevelChunk::touchedSky = false;

LevelChunk::LevelChunk(Level &level, int_t x, int_t z) : level(level)
{
	this->x = x;
	this->z = z;
}

LevelChunk::LevelChunk(Level &level, const ubyte_t *blocks, int_t x, int_t z) : LevelChunk(level, x, z)
{
	std::copy(blocks, blocks + this->blocks.size(), this->blocks.begin());
}

bool LevelChunk::isAt(int_t x, int_t z)
{
	return (this->x == x) && (this->z == z);
}

int_t LevelChunk::getHeightmap(int_t x, int_t z)
{
	// recalcHeightmapOnly();
	return heightmap[(z * 16) | x];
}

void LevelChunk::recalcBlockLights()
{

}

void LevelChunk::recalcHeightmapOnly()
{
	int_t min = Level::DEPTH - 1;

	for (int_t x = 0; x < 16; x++)
	{
		for (int_t z = 0; z < 16; z++)
		{
			int_t y = Level::DEPTH - 1;
			int_t i = ((x * 16) + z) * Level::DEPTH;
			while (y > 0 && Tile::lightBlock[blocks[i + y - 1]] == 0)
				y--;
			heightmap[(z * 16) | x] = y;
			if (y < min)
				min = y;
		}
	}

	minHeight = min;
	unsaved = true;
}

void LevelChunk::recalcHeightmap()
{
	int_t min = Level::DEPTH - 1;

	for (int_t x = 0; x < 16; x++)
	{
		for (int_t z = 0; z < 16; z++)
		{
			// Calculate height map value
			int_t y = Level::DEPTH - 1;
			int_t i = ((x * 16) + z) * Level::DEPTH;
			while (y > 0 && Tile::lightBlock[blocks[i + y - 1]] == 0)
				y--;
			heightmap[(z * 16) | x] = y;
			if (y < min)
				min = y;

			// Populate sky light values
			if (!level.dimension->hasCeiling)
			{
				int_t light = 15;
				int_t ly = Level::DEPTH - 1;
				do
				{
					light -= Tile::lightBlock[static_cast<ubyte_t>(blocks[i + ly])];
					if (light > 0)
						skyLight.set(x, ly, z, light);
				} while (--ly > 0 && light > 0);
			}
		}
	}

	minHeight = min;

	for (int_t x = 0; x < 16; x++)
		for (int_t z = 0; z < 16; z++)
			lightGaps(x, z);

	unsaved = true;
}

void LevelChunk::lightLava()
{
	int_t my = 32;

	for (int_t x = 0; x < 16; x++)
	{
		for (int_t z = 0; z < 16; z++)
		{
			int_t i = ((x * 16) + z) * Level::DEPTH;

			// Set light emissions
			for (int_t y = 0; y < Level::DEPTH; y++)
			{
				int_t emission = Tile::lightEmission[blocks[i + y]];
				if (emission > 0)
					blockLight.set(x, y, z, emission);
			}

			// Propogate upwards
			int_t light = 15;
			for (int_t y = my - 2; my < Level::DEPTH && light > 0;)
			{
				y++;

				ubyte_t b = blocks[i + y];
				int_t block = Tile::lightBlock[b];
				int_t emission = Tile::lightEmission[b];

				if (block == 0)
					block = 1;
				light -= block;
				if (emission > light)
					light = emission;
				if (light < 0)
					light = 0;

				blockLight.set(x, y, z, light);
			}
		}
	}

	level.updateLight(LightLayer::Block, x * 16, my - 1, z * 16, x * 16 + 16, my + 1, z * 16 + 16);
	unsaved = true;
}

void LevelChunk::lightGaps(int_t x, int_t z)
{
	int_t height = getHeightmap(x, z);
	int_t wx = this->x * 16 + x;
	int_t wz = this->z * 16 + z;
	lightGap(wx - 1, wz, height);
	lightGap(wx + 1, wz, height);
	lightGap(wx, wz - 1, height);
	lightGap(wx, wz + 1, height);
}

void LevelChunk::lightGap(int_t x, int_t z, int_t otherHeight)
{
	int_t levelHeight = level.getHeightmap(x, z);
	if (levelHeight > otherHeight)
	{
		level.updateLight(LightLayer::Sky, x, otherHeight, z, x, levelHeight, z);
		unsaved = true;
	}
	else if (levelHeight < otherHeight)
	{
		level.updateLight(LightLayer::Sky, x, levelHeight, z, x, otherHeight, z);
		unsaved = true;
	}
}

void LevelChunk::recalcHeight(int_t x, int_t y, int_t z)
{
	int_t oldHeight = heightmap[(z * 16) | x];
	int_t newHeight = oldHeight;
	if (y > oldHeight)
		newHeight = y;

	int_t ri = (x * (16 * 128)) | (z * 128);
	while (newHeight > 0 && Tile::lightBlock[blocks[ri + newHeight - 1]] == 0)
		newHeight--;

	if (newHeight == oldHeight)
		return;

	level.lightColumnChanged(x, z, newHeight, oldHeight);
	heightmap[(z * 16) | x] = newHeight;

	if (newHeight < minHeight)
	{
		minHeight = newHeight;
	}
	else
	{
		int_t iy = Level::DEPTH - 1;
		for (int_t x = 0; x < 16; x++)
			for (int_t z = 0; z < 16; z++)
				if (heightmap[(z * 16) | x] < iy)
					iy = heightmap[(z * 16) | x];
		minHeight = iy;
	}

	int_t wx = this->x * 16 + x;
	int_t wz = this->z * 16 + z;
	if (newHeight < oldHeight)
	{
		for (int_t yi = newHeight; yi < oldHeight; yi++)
			skyLight.set(x, yi, z, 15);
	}
	else
	{
		level.updateLight(LightLayer::Sky, wx, oldHeight, wz, wx, newHeight, wz);
		for (int_t yi = oldHeight; yi < newHeight; yi++)
			skyLight.set(x, yi, z, 0);
	}

	// Beta 1.2: Recalculate sky light from newHeight downward (LevelChunk.java:244-257)
	int_t light = 15;
	int_t ly = newHeight;
	int_t yPos = newHeight;
	// Beta: for (var10 = var5; var5 > 0 && var16 > 0; this.skyLight.set(var1, var5, var3, var16))
	while (yPos > 0 && light > 0)
	{
		int_t block = Tile::lightBlock[getTile(x, yPos - 1, z)];
		if (block == 0)
			block = 1;
		light -= block;
		if (light < 0)
			light = 0;
		--yPos;
		skyLight.set(x, yPos, z, light);
	}

	// Beta 1.2: Find actual heightmap position (LevelChunk.java:259-261)
	while (yPos > 0 && Tile::lightBlock[getTile(x, yPos - 1, z)] == 0)
		yPos--;
	if (yPos != ly)
		level.updateLight(LightLayer::Sky, wx - 1, yPos, wz - 1, wx + 1, ly, wz + 1);

	unsaved = true;
}

int_t LevelChunk::getTile(int_t x, int_t y, int_t z)
{
	// Beta: LevelChunk.getTile() - returns block at chunk-local coordinates (LevelChunk.java:270-272)
	// Beta: return this.blocks[var1 << 11 | var3 << 7 | var2] (LevelChunk.java:271)
	// Add bounds checking to prevent array out-of-bounds access
	// Chunk-local coordinates: x, z in [0, 15], y in [0, 127]
	if (x < 0 || x >= 16 || z < 0 || z >= 16 || y < 0 || y >= Level::DEPTH)
		return 0;  // Return air for out-of-bounds coordinates
	
	return blocks[(x * (16 * 128)) | (z * 128) | y];
}

bool LevelChunk::setTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t dataValue)
{
	int_t oldHeight = heightmap[(z * 16) | x];
	int_t oldTile = blocks[(x * (16 * 128)) | (z * 128) | y];
	if (oldTile == tile && this->data.get(x, y, z) == dataValue)
		return false;

	int_t wx = this->x * 16 + x;
	int_t wz = this->z * 16 + z;
	blocks[(x * (16 * 128)) | (z * 128) | y] = static_cast<ubyte_t>(tile);
	this->data.set(x, y, z, static_cast<ubyte_t>(dataValue));
	
	if (oldTile != 0)
		Tile::tiles[oldTile]->onRemove(level, wx, y, wz);

	// Beta 1.2: Heightmap recalculation (LevelChunk.java:290-297)
	if (!level.dimension->hasCeiling)
	{
		if (Tile::lightBlock[tile] != 0)
		{
			if (y >= oldHeight)
				recalcHeight(x, y + 1, z);
		}
		else if (y == oldHeight - 1)
		{
			recalcHeight(x, y, z);
		}
	}

	// Beta 1.2: Update lighting for this block only (LevelChunk.java:299-302)
	// Java: Only updates Sky light if dimension has no ceiling
	if (!level.dimension->hasCeiling)
	{
		level.updateLight(LightLayer::Sky, wx, y, wz, wx, y, wz);
	}
	level.updateLight(LightLayer::Block, wx, y, wz, wx, y, wz);
	lightGaps(x, z);
	if (tile != 0 && !level.isOnline)
		Tile::tiles[tile]->onPlace(level, wx, y, wz);

	unsaved = true;
	return true;
}

bool LevelChunk::setTile(int_t x, int_t y, int_t z, int_t tile)
{
	int_t oldHeight = heightmap[(z * 16) | x];
	int_t oldTile = blocks[(x * (16 * 128)) | (z * 128) | y];
	if (oldTile == tile)
		return false;

	int_t wx = this->x * 16 + x;
	int_t wz = this->z * 16 + z;
	blocks[(x * (16 * 128)) | (z * 128) | y] = static_cast<ubyte_t>(tile);
	if (oldTile != 0)
		Tile::tiles[oldTile]->onRemove(level, wx, y, wz);
	data.set(x, y, z, 0);

	if (Tile::lightBlock[tile] != 0)
	{
		if (y >= oldHeight)
			recalcHeight(x, y + 1, z);
	}
	else if (y == oldHeight - 1)
	{
		recalcHeight(x, y, z);
	}

	level.updateLight(LightLayer::Sky, wx, y, wz, wx, y, wz);
	level.updateLight(LightLayer::Block, wx, y, wz, wx, y, wz);
	lightGaps(x, z);
	if (tile != 0 && !level.isOnline)
		Tile::tiles[tile]->onPlace(level, wx, y, wz);

	unsaved = true;
	return true;
}

int_t LevelChunk::getData(int_t x, int_t y, int_t z)
{
	return data.get(x, y, z);
}

void LevelChunk::setData(int_t x, int_t y, int_t z, int_t data)
{
	unsaved = true;
	this->data.set(x, y, z, data);
}

int_t LevelChunk::getBrightness(int_t lightLayer, int_t x, int_t y, int_t z)
{
	if (lightLayer == LightLayer::Sky)
		return skyLight.get(x, y, z);
	if (lightLayer == LightLayer::Block)
		return blockLight.get(x, y, z);
	return 0;
}

void LevelChunk::setBrightness(int_t lightLayer, int_t x, int_t y, int_t z, int_t brightness)
{
	unsaved = true;
	if (lightLayer == LightLayer::Sky)
		skyLight.set(x, y, z, brightness);
	else if (lightLayer == LightLayer::Block)
		blockLight.set(x, y, z, brightness);
}

int_t LevelChunk::getRawBrightness(int_t x, int_t y, int_t z, int_t darken)
{
	int_t light = skyLight.get(x, y, z);
	if (light > 0)
		touchedSky = true;
	light -= darken;

	int_t lightBlock = blockLight.get(x, y, z);
	if (lightBlock > light)
		light = lightBlock;

	return light;
}

void LevelChunk::addEntity(std::shared_ptr<Entity> entity)
{
	// Beta: LevelChunk.addEntity() - adds entity to appropriate entityBlocks slot (LevelChunk.java:394-420)
	int_t y = Mth::floor(entity->y / 16.0);
	if (y < 0)
		y = 0;
	if (y >= static_cast<int_t>(entityBlocks.size()))
		y = static_cast<int_t>(entityBlocks.size()) - 1;
	
	entity->inChunk = true;  // Beta: var1.inChunk = true (LevelChunk.java:412)
	entity->xChunk = this->x;  // Beta: var1.xChunk = this.x (LevelChunk.java:413)
	entity->yChunk = y;  // Beta: var1.yChunk = var4 (LevelChunk.java:414)
	entity->zChunk = this->z;  // Beta: var1.zChunk = this.z (LevelChunk.java:415)
	
	entityBlocks[y].insert(entity);
	lastSaveHadEntities = true;  // Beta: this.lastSaveHadEntities = true (LevelChunk.java:395)
}

void LevelChunk::removeEntity(std::shared_ptr<Entity> entity)
{
	// Beta: LevelChunk.removeEntity() - removes entity from entityBlocks (LevelChunk.java:377-389)
	int_t y = Mth::floor(entity->y / 16.0);
	if (y < 0)
		y = 0;
	if (y >= static_cast<int_t>(entityBlocks.size()))
		y = static_cast<int_t>(entityBlocks.size()) - 1;
	entityBlocks[y].erase(entity);
}

void LevelChunk::removeEntity(std::shared_ptr<Entity> entity, int_t y)
{

}

bool LevelChunk::isSkyLit(int_t x, int_t y, int_t z)
{
	return y >= heightmap[(z * 16) | x];
}

void LevelChunk::skyBrightnessChanged()
{
	int_t x0 = x * 16;
	int_t y0 = minHeight - 16;
	int_t z0 = z * 16;
	int_t x1 = x * 16 + 16;
	int_t y1 = Level::DEPTH;
	int_t z1 = z * 16 + 16;
	level.setTilesDirty(x0, y0, z0, x1, y1, z1);
}

std::shared_ptr<TileEntity> LevelChunk::getTileEntity(int_t x, int_t y, int_t z)
{
	// Beta: LevelChunk.getTileEntity() - gets tile entity from map or creates it (LevelChunk.java:449-464)
	TilePos pos(x, y, z);  // Beta: TilePos var4 = new TilePos(var1, var2, var3) (LevelChunk.java:450)
	auto it = tileEntities.find(pos);
	if (it != tileEntities.end())
	{
		return it->second;  // Beta: return var5 (LevelChunk.java:463)
	}
	
	// Beta: if (var5 == null) (LevelChunk.java:452)
	int_t tileId = getTile(x, y, z);  // Beta: int var6 = this.getTile(var1, var2, var3) (LevelChunk.java:453)
	if (!Tile::isEntityTile[tileId])  // Beta: if (!Tile.isEntityTile[var6]) (LevelChunk.java:454)
	{
		return nullptr;  // Beta: return null (LevelChunk.java:455)
	}
	
	// Beta: EntityTile var7 = (EntityTile)Tile.tiles[var6] (LevelChunk.java:458)
	// Beta: var7.onPlace(this.level, this.x * 16 + var1, var2, this.z * 16 + var3) (LevelChunk.java:459)
	Tile *tile = Tile::tiles[tileId];
	if (tile != nullptr)
	{
		int_t wx = this->x * 16 + x;
		int_t wz = this->z * 16 + z;
		tile->onPlace(level, wx, y, wz);  // This will create the tile entity via setTileEntity
		
		// Beta: var5 = this.tileEntities.get(var4) (LevelChunk.java:460)
		it = tileEntities.find(pos);
		if (it != tileEntities.end())
		{
			return it->second;
		}
	}
	
	return nullptr;
}

void LevelChunk::addTileEntity(std::shared_ptr<TileEntity> tileEntity)
{
	// Beta: LevelChunk.addTileEntity() - converts world coords to chunk coords (LevelChunk.java:466-471)
	int_t lx = tileEntity->x - this->x * 16;  // Beta: int var2 = var1.x - this.x * 16 (LevelChunk.java:467)
	int_t ly = tileEntity->y;  // Beta: int var3 = var1.y (LevelChunk.java:468)
	int_t lz = tileEntity->z - this->z * 16;  // Beta: int var4 = var1.z - this.z * 16 (LevelChunk.java:469)
	setTileEntity(lx, ly, lz, tileEntity);  // Beta: this.setTileEntity(var2, var3, var4, var1) (LevelChunk.java:470)
}

void LevelChunk::setTileEntity(int_t x, int_t y, int_t z, std::shared_ptr<TileEntity> tileEntity)
{
	// Beta: LevelChunk.setTileEntity() - sets tile entity in map and level list (LevelChunk.java:473-492)
	TilePos pos(x, y, z);  // Beta: TilePos var5 = new TilePos(var1, var2, var3) (LevelChunk.java:474)
	// Beta: var4.level = this.level (LevelChunk.java:475)
	// Note: In C++, level is a reference, but TileEntity needs shared_ptr
	// We need to get the shared_ptr from Level if it exists, or create a non-owning one
	// For now, we'll need to handle this differently - Level should provide a way to get shared_ptr
	tileEntity->level = std::shared_ptr<Level>(&level, [](Level*) {});  // Non-owning shared_ptr
	tileEntity->x = this->x * 16 + x;  // Beta: var4.x = this.x * 16 + var1 (LevelChunk.java:476)
	tileEntity->y = y;  // Beta: var4.y = var2 (LevelChunk.java:477)
	tileEntity->z = this->z * 16 + z;  // Beta: var4.z = this.z * 16 + var3 (LevelChunk.java:478)
	
	int_t tileId = getTile(x, y, z);  // Beta: this.getTile(var1, var2, var3) (LevelChunk.java:479)
	if (tileId != 0 && Tile::isEntityTile[tileId])  // Beta: if (this.getTile(var1, var2, var3) != 0 && Tile.tiles[this.getTile(var1, var2, var3)] instanceof EntityTile) (LevelChunk.java:479)
	{
		if (loaded)  // Beta: if (this.loaded) (LevelChunk.java:480)
		{
			auto oldIt = tileEntities.find(pos);
			if (oldIt != tileEntities.end())  // Beta: if (this.tileEntities.get(var5) != null) (LevelChunk.java:481)
			{
				level.tileEntityList.erase(oldIt->second);  // Beta: this.level.tileEntityList.remove(this.tileEntities.get(var5)) (LevelChunk.java:482)
			}
			
			level.tileEntityList.insert(tileEntity);  // Beta: this.level.tileEntityList.add(var4) (LevelChunk.java:485)
		}
		
		tileEntities[pos] = tileEntity;  // Beta: this.tileEntities.put(var5, var4) (LevelChunk.java:488)
	}
	else
	{
		std::cout << "Attempted to place a tile entity where there was no entity tile!" << std::endl;  // Beta: System.out.println(...) (LevelChunk.java:490)
	}
}

void LevelChunk::removeTileEntity(int_t x, int_t y, int_t z)
{
	// Beta: LevelChunk.removeTileEntity() - removes from map and level list (LevelChunk.java:494-499)
	TilePos pos(x, y, z);  // Beta: TilePos var4 = new TilePos(var1, var2, var3) (LevelChunk.java:495)
	if (loaded)  // Beta: if (this.loaded) (LevelChunk.java:496)
	{
		auto it = tileEntities.find(pos);
		if (it != tileEntities.end())
		{
			level.tileEntityList.erase(it->second);  // Beta: this.level.tileEntityList.remove(...) (LevelChunk.java:497)
			tileEntities.erase(it);  // Beta: this.tileEntities.remove(var4) (LevelChunk.java:497)
		}
	}
	else
	{
		tileEntities.erase(pos);
	}
}

void LevelChunk::load()
{

}

void LevelChunk::unload()
{
	loaded = false;
	for (auto &i : tileEntities)
		level.tileEntityList.erase(i.second);
	for (auto &b : entityBlocks)
		level.removeEntities(b);
}

void LevelChunk::markUnsaved()
{
	unsaved = true;
}

void LevelChunk::getEntities(Entity *ignore, AABB &aabb, std::vector<std::shared_ptr<Entity>> &entities)
{
	// Beta: LevelChunk.getEntities() - finds entities in AABB range (LevelChunk.java:523-544)
	int_t y0 = Mth::floor((aabb.y0 - 2.0) / 16.0);
	int_t y1 = Mth::floor((aabb.y1 + 2.0) / 16.0);
	if (y0 < 0)
		y0 = 0;
	if (y1 >= static_cast<int_t>(entityBlocks.size()))
		y1 = static_cast<int_t>(entityBlocks.size()) - 1;

	for (int_t y = y0; y <= y1; y++)
	{
		for (const auto &entity : entityBlocks[y])
		{
			if (entity.get() != ignore && entity->bb.intersects(aabb))  // Beta: var9 != var1 && var9.bb.intersects(var2) (LevelChunk.java:539)
			{
				entities.push_back(entity);
			}
		}
	}
}

void LevelChunk::getEntitiesOfCondition(bool (*condition)(Entity &), AABB &aabb, std::vector<std::shared_ptr<Entity>> &entities)
{

}

int_t LevelChunk::countEntities()
{
	int_t count = 0;
	for (auto &b : entityBlocks)
		count += b.size();
	return count;
}

bool LevelChunk::shouldSave(bool force)
{
	if (dontSave)
		return false;

	if (force)
	{
		if (lastSaveHadEntities && level.time != lastSaveTime)
			return true;
	}
	else
	{
		if (lastSaveHadEntities && level.time >= lastSaveTime + 600)
			return true;
	}

	return unsaved;
}

void LevelChunk::setBlocks(byte_t *blocks, int_t y)
{

}

int_t LevelChunk::getBlocksAndData(byte_t *out, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	return 0;
}

int_t LevelChunk::setBlocksAndData(byte_t *in, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	return 0;
}

// Alpha 1.2.6: Chunk.func_1004_a - EXACT implementation
// func_1004_a(byte[] var1, int var2, int var3, int var4, int var5, int var6, int var7, int var8)
// Returns the new offset for the next chunk
int_t LevelChunk::setBlocksAndDataFromPacket(byte_t *data, int_t xStart, int_t yStart, int_t zStart, 
                                              int_t xEnd, int_t yEnd, int_t zEnd, int_t offset)
{
	// Alpha 1.2.6: Copy blocks first (Chunk.java:554-561)
	for (int_t lx = xStart; lx < xEnd; ++lx)
	{
		for (int_t lz = zStart; lz < zEnd; ++lz)
		{
			int_t idx = (lx << 11) | (lz << 7) | yStart;
			int_t count = yEnd - yStart;
			std::copy(data + offset, data + offset + count, blocks.begin() + idx);
			offset += count;
		}
	}
	
	// Alpha 1.2.6: Generate heightmap (Chunk.java:563)
	recalcHeightmapOnly();
	
	// Alpha 1.2.6: Copy data (nibbles) (Chunk.java:565-572)
	for (int_t lx = xStart; lx < xEnd; ++lx)
	{
		for (int_t lz = zStart; lz < zEnd; ++lz)
		{
			int_t idx = ((lx << 11) | (lz << 7) | yStart) >> 1;
			int_t count = (yEnd - yStart) / 2;
			std::copy(data + offset, data + offset + count, this->data.data.begin() + idx);
			offset += count;
		}
	}
	
	// Alpha 1.2.6: Copy blockLight (nibbles) (Chunk.java:574-581)
	for (int_t lx = xStart; lx < xEnd; ++lx)
	{
		for (int_t lz = zStart; lz < zEnd; ++lz)
		{
			int_t idx = ((lx << 11) | (lz << 7) | yStart) >> 1;
			int_t count = (yEnd - yStart) / 2;
			std::copy(data + offset, data + offset + count, blockLight.data.begin() + idx);
			offset += count;
		}
	}
	
	// Alpha 1.2.6: Copy skyLight (nibbles) (Chunk.java:583-590)
	for (int_t lx = xStart; lx < xEnd; ++lx)
	{
		for (int_t lz = zStart; lz < zEnd; ++lz)
		{
			int_t idx = ((lx << 11) | (lz << 7) | yStart) >> 1;
			int_t count = (yEnd - yStart) / 2;
			std::copy(data + offset, data + offset + count, skyLight.data.begin() + idx);
			offset += count;
		}
	}
	
	// Mark chunk as loaded
	loaded = true;
	unsaved = true;
	
	// Alpha 1.2.6: Return new offset for next chunk (Chunk.java:592)
	return offset;
}

Random LevelChunk::getRandom(long_t seed)
{
	return Random();
}

bool LevelChunk::isEmpty()
{
	return false;
}
