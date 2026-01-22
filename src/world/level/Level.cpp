#include "world/level/Level.h"

#include <algorithm>
#include <iostream>

#include "world/level/ChunkPos.h"
#include "world/level/LightLayer.h"
#include "util/Mth.h"

#include "nbt/NbtIo.h"

#include "world/level/chunk/ChunkCache.h"
#include "world/level/material/GasMaterial.h"
#include "world/level/NextTickListEntry.h"
#include "world/level/MobSpawner.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/FireTile.h"
#include "world/level/tile/FarmTile.h"
#include "world/phys/Vec3.h"
#include "world/entity/Entity.h"
#include "world/entity/MobCategory.h"
#include "world/entity/monster/Monster.h"
#include "world/entity/animal/Animal.h"
#include "world/level/tile/StoneSlabTile.h"
#include "world/level/pathfinder/Path.h"
#include "world/level/pathfinder/PathFinder.h"
#include "world/level/Explosion.h"

#include "java/File.h"
#include "java/IOUtil.h"

#include "util/Mth.h"

int_t Level::maxLoop = 0;

std::shared_ptr<CompoundTag> Level::getDataTagFor(File &workingDirectory, const jstring &name)
{
	std::unique_ptr<File> saves(File::open(workingDirectory, u"saves"));
	std::unique_ptr<File> world(File::open(*saves, name));
	if (!world->exists())
		return nullptr;

	std::unique_ptr<File> levelDat(File::open(*world, u"level.dat"));
	if (!levelDat->exists())
		return nullptr;

	std::unique_ptr<std::istream> is(levelDat->toStreamIn());
	std::unique_ptr<CompoundTag> tag(NbtIo::readCompressed(*is));
	return tag->getCompound(u"Data");
}

void Level::deleteLevel(File &workingDirectory, const jstring &name)
{
	std::unique_ptr<File> saves(File::open(workingDirectory, u"saves"));
	std::unique_ptr<File> world(File::open(*saves, name));
	if (!world->exists())
		return;

	auto level_files = world->listFiles();
	deleteRecursive(level_files);
	world->remove();
}

void Level::deleteRecursive(std::vector<std::unique_ptr<File>> &files)
{
	for (auto &i : files)
	{
		if (i->isDirectory())
		{
			auto i_files = i->listFiles();
			deleteRecursive(i_files);
		}
		i->remove();
	}
}

BiomeSource &Level::getBiomeSource()
{
	return *dimension->biomeSource;
}

long_t Level::getLevelSize(File &workingDirectory, const jstring &name)
{
	std::unique_ptr<File> saves(File::open(workingDirectory, u"saves"));
	std::unique_ptr<File> world(File::open(*saves, name));
	if (!world->exists())
		return 0;
	auto level_files = world->listFiles();
	return calcSize(level_files);
}

long_t Level::calcSize(std::vector<std::unique_ptr<File>> &files)
{
	long_t sizeOnDisk = 0;
	for (auto &i : files)
	{
		if (i->isDirectory())
		{
			auto i_files = i->listFiles();
			sizeOnDisk += calcSize(i_files);
		}
		else
		{
			sizeOnDisk += i->length();
		}
	}
	return sizeOnDisk;
}

Level::Level(File *workingDirectory, const jstring &name) : Level(workingDirectory, name, Random().nextLong())
{

}

Level::Level(const jstring &name, int_t dimension, long_t seed)
{
	this->name = name;
	this->seed = seed;
	this->dimension.reset(Dimension::getNew(*this, dimension));
	this->chunkSource.reset(createChunkSource(dir));
	updateSkyBrightness();
}

Level::Level(Level &level, int_t dimension)
{
	this->sessionId = level.sessionId;
	this->workDir = std::move(level.workDir);
	this->dir = std::move(level.dir);
	this->seed = level.seed;
	this->time = level.time;
	this->xSpawn = level.xSpawn;
	this->ySpawn = level.ySpawn;
	this->zSpawn = level.zSpawn;
	this->sizeOnDisk = level.sizeOnDisk;
	this->dimension.reset(Dimension::getNew(*this, dimension));
	this->chunkSource.reset(createChunkSource(dir));
	updateSkyBrightness();
}

Level::Level(File *workingDirectory, const jstring &name, long_t seed) : Level(workingDirectory, name, seed, Dimension::Id_None)
{

}

Level::Level(File *workingDirectory, const jstring &name, long_t seed, int_t dimension)
{
	this->workDir.reset(workingDirectory);
	this->name = name;

	workingDirectory->mkdirs();
	dir.reset(File::open(*workingDirectory, name));
	dir->mkdirs();
	
	{
		std::unique_ptr<File> session_lock(File::open(*dir, u"session.lock"));
		std::unique_ptr<std::ostream> os(session_lock->toStreamOut());
		if (!os)
			throw std::runtime_error("Failed to check session lock, aborting");
		IOUtil::writeLong(*os, sessionId);
	}

	int_t new_dimension = Dimension::Id_Normal;

	std::unique_ptr<File> fileDat(File::open(*dir, u"level.dat"));
	isNew = !fileDat->exists();

	if (fileDat->exists())
	{
		std::shared_ptr<CompoundTag> root(NbtIo::readCompressed(*std::unique_ptr<std::istream>(fileDat->toStreamIn())));
		std::shared_ptr<CompoundTag> data(root->getCompound(u"Data"));
		seed = data->getLong(u"RandomSeed");
		xSpawn = data->getInt(u"SpawnX");
		ySpawn = data->getInt(u"SpawnY");
		zSpawn = data->getInt(u"SpawnZ");
		time = data->getLong(u"Time");
		sizeOnDisk = data->getLong(u"SizeOnDisk");

		if (data->contains(u"Player"))
		{
			loadedPlayerTag = data->getCompound(u"Player");

			int_t tag_dimension = loadedPlayerTag->getInt(u"Dimension");
			if (tag_dimension == Dimension::Id_Hell)
				dimension = tag_dimension;
		}
	}

	if (dimension != Dimension::Id_None)
		new_dimension = dimension;

	bool findSpawn = false;
	if (this->seed == 0)
	{
		this->seed = seed;
		findSpawn = true;
	}

	this->dimension.reset(Dimension::getNew(*this, new_dimension));
	this->chunkSource.reset(createChunkSource(dir));

	if (findSpawn)
	{
		isFindingSpawn = true;
		xSpawn = 0;
		ySpawn = 64;
		zSpawn = 0;
		while (!this->dimension->isValidSpawn(xSpawn, zSpawn))
		{
			xSpawn += random.nextInt(64) - random.nextInt(64);
			zSpawn += random.nextInt(64) - random.nextInt(64);
		}
		isFindingSpawn = false;
	}
}

ChunkSource *Level::createChunkSource(std::shared_ptr<File> dir)
{
	// Handle null dir (e.g., for multiplayer levels that don't use file storage)
	// Also handle null dimension (shouldn't happen, but prevents crash)
	if (dimension == nullptr)
	{
		throw std::runtime_error("Dimension is null in createChunkSource");
	}
	ChunkStorage* storage = (dir != nullptr) ? dimension->createStorage(dir) : nullptr;
	return new ChunkCache(*this, storage, dimension->createRandomLevelSource());
}

void Level::validateSpawn()
{

}

int_t Level::getTopTile(int_t x, int_t z)
{
	byte_t y;
	for (y = SEA_LEVEL; !isEmptyTile(x, y + 1, z); y++);
	return getTile(x, y, z);
}

void Level::clearLoadedPlayerData()
{
	// newb12: clearLoadedPlayerData() - clears loaded player tag so respawn uses spawn point (Level.java:286-287)
	loadedPlayerTag = nullptr;
}

void Level::loadPlayer(std::shared_ptr<Player> player)
{
	if (loadedPlayerTag != nullptr)
	{
		player->load(*loadedPlayerTag);
		loadedPlayerTag = nullptr;
	}

	if (chunkSource->isChunkCache())
	{
		ChunkCache &chunkCache = static_cast<ChunkCache &>(*chunkSource);
		int_t x = Mth::floor(static_cast<float>(static_cast<int_t>(player->x))) >> 4;
		int_t z = Mth::floor(static_cast<float>(static_cast<int_t>(player->z))) >> 4;
		chunkCache.centerOn(x, z);
	}

	addEntity(player);
}

void Level::save(bool force, std::shared_ptr<ProgressListener> progressRenderer)
{
	if (!chunkSource->shouldSave())
		return;

	if (progressRenderer != nullptr)
		progressRenderer->progressStartNoAbort(u"Saving level");
	saveLevelData();
	if (progressRenderer != nullptr)
		progressRenderer->progressStage(u"Saving chunks");
	chunkSource->save(force, progressRenderer);
}

void Level::saveLevelData()
{
	checkSession();

	std::shared_ptr<CompoundTag> data = Util::make_shared<CompoundTag>();
	data->putLong(u"RandomSeed", seed);
	data->putInt(u"SpawnX", xSpawn);
	data->putInt(u"SpawnY", ySpawn);
	data->putInt(u"SpawnZ", zSpawn);
	data->putLong(u"Time", time);
	data->putLong(u"SizeOnDisk", sizeOnDisk);
	data->putLong(u"LastPlayed", System::currentTimeMillis());

	std::shared_ptr<Player> player;
	if (!players.empty())
		player = players[0];

	if (player != nullptr)
	{
		std::shared_ptr<CompoundTag> playerTag = Util::make_shared<CompoundTag>();
		player->saveWithoutId(*playerTag);
		data->put(u"Player", playerTag);
	}

	std::shared_ptr<CompoundTag> root = Util::make_shared<CompoundTag>();
	root->put(u"Data", data);

	std::unique_ptr<File> fileNewDat(File::open(*dir, u"level.dat_new"));
	std::unique_ptr<File> fileOldDat(File::open(*dir, u"level.dat_old"));
	std::unique_ptr<File> fileDat(File::open(*dir, u"level.dat"));

	std::unique_ptr<std::ostream> os(fileNewDat->toStreamOut());
	NbtIo::writeCompressed(*root, *os);
	os->flush();
	os.reset();

	if (fileOldDat->exists())
		fileOldDat->remove();
	fileDat->renameTo(*fileOldDat);
	if (fileDat->exists())
		fileDat->remove();
	fileNewDat->renameTo(*fileDat);
	if (fileNewDat->exists())
		fileNewDat->remove();
}

bool Level::pauseSave(int_t saveStep)
{
	if (!chunkSource->shouldSave())
		return true;
	if (saveStep == 0)
		saveLevelData();
	return chunkSource->save(false, nullptr);
}

int_t Level::getTile(int_t x, int_t y, int_t z)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return 0;
	if (y < 0)
		return 0;
	if (y >= DEPTH)
		return 0;
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	return chunk->getTile(x & 0xF, y, z & 0xF);
}

bool Level::isEmptyTile(int_t x, int_t y, int_t z)
{
	return getTile(x, y, z) == 0;
}

bool Level::hasChunkAt(int_t x, int_t y, int_t z)
{
	return y >= 0 && y < DEPTH && hasChunk(x >> 4, z >> 4);
}

bool Level::hasChunksAt(int_t x, int_t y, int_t z, int_t d)
{
	return hasChunksAt(x - d, y - d, z - d, x + d, y + d, z + d);
}

bool Level::hasChunksAt(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	if (y1 < 0 || y0 >= DEPTH)
		return false;

	x0 >>= 4;
	y0 >>= 4;
	z0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	z1 >>= 4;

	for (int_t cx = x0; cx <= x1; cx++)
		for (int_t cz = z0; cz <= z1; cz++)
			if (!hasChunk(cx, cz))
				return false;
	return true;
}

bool Level::hasChunk(int_t xc, int_t zc)
{
	return chunkSource->hasChunk(xc, zc);
}

std::shared_ptr<LevelChunk> Level::getChunkAt(int_t x, int_t z)
{
	return getChunk(x >> 4, z >> 4);
}

std::shared_ptr<LevelChunk> Level::getChunk(int_t xc, int_t zc)
{
	return chunkSource->getChunk(xc, zc);
}

bool Level::setTileAndDataNoUpdate(int_t x, int_t y, int_t z, int_t tile, int_t data)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return false;
	if (y < 0)
		return false;
	if (y >= DEPTH)
		return false;
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	return chunk->setTileAndData(x & 0xF, y, z & 0xF, tile, data);
}

bool Level::setTileNoUpdate(int_t x, int_t y, int_t z, int_t tile)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return false;
	if (y < 0)
		return false;
	if (y >= DEPTH)
		return false;
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	return chunk->setTile(x & 0xF, y, z & 0xF, tile);
}

const Material &Level::getMaterial(int_t x, int_t y, int_t z)
{
	int_t tile = getTile(x, y, z);
	return (tile == 0) ? Material::air : Tile::tiles[tile]->material;
}

int_t Level::getData(int_t x, int_t y, int_t z)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return 0;
	if (y < 0)
		return 0;
	if (y >= DEPTH)
		return 0;
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	return chunk->getData(x & 0xF, y, z & 0xF);
}

void Level::setData(int_t x, int_t y, int_t z, int_t data)
{
	if (setDataNoUpdate(x, y, z, data))
		tileUpdated(x, y, z, getTile(x, y, z));
}

bool Level::setDataNoUpdate(int_t x, int_t y, int_t z, int_t data)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return false;
	if (y < 0)
		return false;
	if (y >= DEPTH)
		return false;
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	chunk->setData(x & 0xF, y, z & 0xF, data);
	return true;
}

bool Level::setTile(int_t x, int_t y, int_t z, int_t tile)
{
	if (setTileNoUpdate(x, y, z, tile))
	{
		tileUpdated(x, y, z, tile);
		return true;
	}
	return false;
}

bool Level::setTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t data)
{
	if (setTileAndDataNoUpdate(x, y, z, tile, data))
	{
		tileUpdated(x, y, z, tile);
		return true;
	}
	return false;
}

// Beta: Level.mayPlace() - checks if block can be placed (Level.java:1833-1848)
bool Level::mayPlace(int_t tileId, int_t x, int_t y, int_t z, bool flag)
{
	// Beta: int var6 = this.getTile(var2, var3, var4) (Level.java:1834)
	int_t var6 = getTile(x, y, z);
	
	// Beta: Tile var7 = Tile.tiles[var6] (Level.java:1835)
	Tile *var7 = Tile::tiles[var6];
	
	// Beta: Tile var8 = Tile.tiles[var1] (Level.java:1836)
	Tile *var8 = Tile::tiles[tileId];
	if (var8 == nullptr)
		return false;
	
	// Beta: AABB var9 = var8.getAABB(this, var2, var3, var4) (Level.java:1837)
	AABB *var9 = var8->getAABB(*this, x, y, z);
	
	// Beta: if (var5) var9 = null (Level.java:1838-1839)
	if (flag)
		var9 = nullptr;
	
	// Beta: if (var9 != null && !this.isUnobstructed(var9)) return false (Level.java:1842-1843)
	if (var9 != nullptr && !isUnobstructed(*var9))
		return false;
	
	// Beta: return var7 == Tile.water || var7 == Tile.calmWater || var7 == Tile.lava || var7 == Tile.calmLava || var7 == Tile.fire || var7 == Tile.topSnow ? true : var1 > 0 && var7 == null && var8.mayPlace(this, var2, var3, var4) (Level.java:1845-1847)
	// Note: In C++, we compare by ID since pointer types differ (Tile* vs FluidFlowingTile*)
	// Tile IDs: water=8, calmWater=9, lava=10, calmLava=11, snow=78 (from Tile.cpp)
	// Note: Tile::fire doesn't exist in C++ yet, so we skip that check for now
	if (var6 == 8 || var6 == 9 || var6 == 10 || var6 == 11 || var6 == 78)
		return true;
	
	// Beta: var1 > 0 && var7 == null && var8.mayPlace(this, var2, var3, var4) (Level.java:1847)
	return tileId > 0 && var7 == nullptr && var8->mayPlace(*this, x, y, z);
}

void Level::sendTileUpdated(int_t x, int_t y, int_t z)
{
	for (auto &l : listeners)
		l->tileChanged(x, y, z);
}

void Level::tileUpdated(int_t x, int_t y, int_t z, int_t tile)
{
	sendTileUpdated(x, y, z);
	updateNeighborsAt(x, y, z, tile);
}

void Level::lightColumnChanged(int_t x, int_t z, int_t y0, int_t y1)
{
	if (y0 > y1)
	{
		int_t temp = y0;
		y0 = y1;
		y1 = temp;
	}
	setTilesDirty(x, y0, z, x, y1, z);
}

void Level::setTileDirty(int_t x, int_t y, int_t z)
{
	for (auto &l : listeners)
		l->setTilesDirty(x, y, z, x, y, z);
}

void Level::setTilesDirty(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	for (auto &l : listeners)
		l->setTilesDirty(x0, y0, z0, x1, y1, z1);
}

void Level::swap(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	int_t t0 = getTile(x0, y0, z0);
	int_t d0 = getData(x0, y0, z0);
	int_t t1 = getTile(x1, y1, z1);
	int_t d1 = getData(x1, y1, z1);

	setTileAndDataNoUpdate(x0, y0, z0, t1, d1);
	setTileAndDataNoUpdate(x1, y1, z1, t0, d0);
	updateNeighborsAt(x0, y0, z0, t1);
	updateNeighborsAt(x1, y1, z1, t0);
}

void Level::updateNeighborsAt(int_t x, int_t y, int_t z, int_t tile)
{
	neighborChanged(x - 1, y, z, tile);
	neighborChanged(x + 1, y, z, tile);
	neighborChanged(x, y - 1, z, tile);
	neighborChanged(x, y + 1, z, tile);
	neighborChanged(x, y, z - 1, tile);
	neighborChanged(x, y, z + 1, tile);
}

bool Level::getDirectSignal(int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: Level.getDirectSignal() - gets direct redstone signal (Level.java:1885-1888)
	int_t tileId = getTile(x, y, z);
	return tileId == 0 ? false : Tile::tiles[tileId]->getDirectSignal(*this, x, y, z, facing);
}

bool Level::getSignal(int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: Level.getSignal() - gets redstone signal strength (Level.java:1904-1912)
	if (isSolidTile(x, y, z))
	{
		// Beta: return this.hasDirectSignal(var1, var2, var3) (Level.java:1906)
		// hasDirectSignal checks all 6 directions for direct signals
		if (getDirectSignal(x, y - 1, z, 0)) return true;
		if (getDirectSignal(x, y + 1, z, 1)) return true;
		if (getDirectSignal(x, y, z - 1, 2)) return true;
		if (getDirectSignal(x, y, z + 1, 3)) return true;
		if (getDirectSignal(x - 1, y, z, 4)) return true;
		if (getDirectSignal(x + 1, y, z, 5)) return true;
		return false;
	}
	else
	{
		int_t tileId = getTile(x, y, z);
		return tileId == 0 ? false : Tile::tiles[tileId]->getSignal(*this, x, y, z, facing);
	}
}

bool Level::hasNeighborSignal(int_t x, int_t y, int_t z)
{
	// Beta: Level.hasNeighborSignal() - checks if any neighbor has signal (Level.java:1913-1925)
	if (getSignal(x, y - 1, z, 0))
	{
		return true;
	}
	else if (getSignal(x, y + 1, z, 1))
	{
		return true;
	}
	else if (getSignal(x, y, z - 1, 2))
	{
		return true;
	}
	else if (getSignal(x, y, z + 1, 3))
	{
		return true;
	}
	else
	{
		return getSignal(x - 1, y, z, 4) ? true : getSignal(x + 1, y, z, 5);
	}
}

void Level::neighborChanged(int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: Level.neighborChanged() (Level.java:582-588)
	// Beta: if (!this.noNeighborUpdate && !this.isOnline) (Level.java:583)
	if (!noNeighborUpdate && !isOnline)
	{
		// Beta: Tile var5 = Tile.tiles[this.getTile(var1, var2, var3)] (Level.java:584)
		Tile *ptile = Tile::tiles[getTile(x, y, z)];
		// Beta: if (var5 != null) var5.neighborChanged(this, var1, var2, var3, var4) (Level.java:585-586)
		if (ptile != nullptr)
		{
			ptile->neighborChanged(*this, x, y, z, tile);
		}
	}
}

bool Level::canSeeSky(int_t x, int_t y, int_t z)
{
	return getChunk(x >> 4, z >> 4)->isSkyLit(x & 0xF, y, z & 0xF);
}

int_t Level::getRawBrightness(int_t x, int_t y, int_t z)
{
	return getRawBrightness(x, y, z, true);
}

int_t Level::getRawBrightness(int_t x, int_t y, int_t z, bool neighbors)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return 15;

	if (neighbors)
	{
		int_t tile = getTile(x, y, z);
		// Beta: Special light handling for stoneSlabHalf and farmland (Level.java:603-626)
		// Both use the same lighting logic - check brightness from above and sides, return maximum
		if (tile == Tile::stoneSlabHalf.id || tile == Tile::farmland.id)
		{
			int_t brightness = getRawBrightness(x, y + 1, z, false);
			int_t bpx = getRawBrightness(x + 1, y, z, false);
			int_t bnx = getRawBrightness(x - 1, y, z, false);
			int_t bpz = getRawBrightness(x, y, z + 1, false);
			int_t bnz = getRawBrightness(x, y, z - 1, false);
			
			if (bpx > brightness) brightness = bpx;
			if (bnx > brightness) brightness = bnx;
			if (bpz > brightness) brightness = bpz;
			if (bnz > brightness) brightness = bnz;
			
			return brightness;
		}
	}

	if (y < 0)
		return 0;

	if (y >= DEPTH)
	{
		int_t l = 15 - skyDarken;
		if (l < 0)
			l = 0;
		return l;
	}

	return getChunk(x >> 4, z >> 4)->getRawBrightness(x & 0xF, y, z & 0xF, skyDarken);
}

bool Level::isSkyLit(int_t x, int_t y, int_t z)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return false;
	if (y < 0)
		return false;
	if (y >= DEPTH)
		return true;
	if (!hasChunk(x >> 4, z >> 4))
		return false;
	return getChunk(x >> 4, z >> 4)->isSkyLit(x & 0xF, y, z & 0xF);
}

int_t Level::getHeightmap(int_t x, int_t z)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return 0;
	if (!hasChunk(x >> 4, z >> 4))
		return 0;
	return getChunk(x >> 4, z >> 4)->getHeightmap(x & 0xF, z & 0xF);
}

void Level::updateLightIfOtherThan(int_t layer, int_t x, int_t y, int_t z, int_t brightness)
{
	if (dimension->hasCeiling && layer == LightLayer::Sky)
		return;
	if (!hasChunkAt(x, y, z))
		return;

	if (layer == LightLayer::Sky)
	{
		if (isSkyLit(x, y, z))
			brightness = 15;
	}
	else if (layer == LightLayer::Block)
	{
		int_t tile = getTile(x, y, z);
		if (Tile::lightEmission[tile] > brightness)
			brightness = Tile::lightEmission[tile];
	}

	if (getBrightness(layer, x, y, z) != brightness)
		updateLight(layer, x, y, z, x, y, z);
}

int_t Level::getBrightness(int_t layer, int_t x, int_t y, int_t z)
{
	if (y < 0 || y >= DEPTH || x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z >= MAX_LEVEL_SIZE)
		return LightLayer::surrounding(layer);

	int_t xc = x >> 4;
	int_t zc = z >> 4;
	if (!hasChunk(xc, zc))
		return 0;

	return getChunk(xc, zc)->getBrightness(layer, x & 0xF, y, z & 0xF);
}

void Level::setBrightness(int_t layer, int_t x, int_t y, int_t z, int_t brightness)
{
	if (x < -MAX_LEVEL_SIZE || z < -MAX_LEVEL_SIZE || x >= MAX_LEVEL_SIZE || z > MAX_LEVEL_SIZE)
		return;
	if (y < 0)
		return;
	if (y >= DEPTH)
		return;
	if (!hasChunk(x >> 4, z >> 4))
		return;

	getChunk(x >> 4, z >> 4)->setBrightness(layer, x & 0xF, y, z & 0xF, brightness);

	for (auto &l : listeners)
		l->tileChanged(x, y, z);
}

float Level::getBrightness(int_t x, int_t y, int_t z)
{
	return dimension->brightnessRamp[getRawBrightness(x, y, z)];
}

bool Level::isDay()
{
	return skyDarken < 4;
}

HitResult Level::clip(Vec3 &from, Vec3 &to)
{
	return clip(from, to, false);
}

HitResult Level::clip(Vec3 &from, Vec3 &to, bool canPickLiquid)
{
	if (std::isnan(from.x) || std::isnan(from.y) || std::isnan(from.z)) return HitResult();
	if (std::isnan(to.x) || std::isnan(to.y) || std::isnan(to.z)) return HitResult();

	int_t x0 = Mth::floor(to.x);
	int_t y0 = Mth::floor(to.y);
	int_t z0 = Mth::floor(to.z);
	int_t x1 = Mth::floor(from.x);
	int_t y1 = Mth::floor(from.y);
	int_t z1 = Mth::floor(from.z);
	int_t steps = 200;

	while (steps-- >= 0)
	{
		if (std::isnan(from.x) || std::isnan(from.y) || std::isnan(from.z)) return HitResult();
		if (x1 == x0 && y1 == y0 && z1 == z0)
			return HitResult();

		double dx = 999.0;
		double dy = 999.0;
		double dz = 999.0;

		if (x0 > x1) dx = x1 + 1.0;
		if (x0 < x1) dx = x1 + 0.0;
		if (y0 > y1) dy = y1 + 1.0;
		if (y0 < y1) dy = y1 + 0.0;
		if (z0 > z1) dz = z1 + 1.0;
		if (z0 < z1) dz = z1 + 0.0;

		double dx2 = 999.0;
		double dy2 = 999.0;
		double dz2 = 999.0;
		double ddx = to.x - from.x;
		double ddy = to.y - from.y;
		double ddz = to.z - from.z;

		if (dx != 999.0) dx2 = (dx - from.x) / ddx;
		if (dy != 999.0) dy2 = (dy - from.y) / ddy;
		if (dz != 999.0) dz2 = (dz - from.z) / ddz;

		Facing face = Facing::DOWN;
		if (dx2 < dy2 && dx2 < dz2)
		{
			if (x0 > x1)
				face = Facing::WEST;
			else
				face = Facing::EAST;

			from.x = dx;
			from.y += ddy * dx2;
			from.z += ddz * dx2;
		}
		else if (dy2 < dz2)
		{
			if (y0 > y1)
				face = Facing::DOWN;
			else
				face = Facing::UP;

			from.x += ddx * dy2;
			from.y = dy;
			from.z += ddz * dy2;
		}
		else
		{
			if (z0 > z1)
				face = Facing::NORTH;
			else
				face = Facing::SOUTH;

			from.x += ddx * dz2;
			from.y += ddy * dz2;
			from.z = dz;
		}

		Vec3 *newVec = Vec3::newTemp(from.x, from.y, from.z);

		x1 = newVec->x = Mth::floor(from.x);
		if (face == Facing::EAST)
		{
			x1--;
			newVec->x++;
		}

		y1 = newVec->y = Mth::floor(from.y);
		if (face == Facing::UP)
		{
			y1--;
			newVec->y++;
		}

		z1 = newVec->z = Mth::floor(from.z);
		if (face == Facing::SOUTH)
		{
			z1--;
			newVec->z++;
		}

		int_t tile = getTile(x1, y1, z1);
		int_t data = getData(x1, y1, z1);
		Tile &tt = *Tile::tiles[tile];
		if (tile > 0 && tt.mayPick(data, canPickLiquid))
		{
			HitResult hitResult = tt.clip(*this, x1, y1, z1, from, to);
			if (hitResult.type != HitResult::Type::NONE)
				return hitResult;
		}
	}

	return HitResult();
}


bool Level::addEntity(std::shared_ptr<Entity> entity)
{
	int_t cx = Mth::floor(entity->x / 16.0);
	int_t cz = Mth::floor(entity->z / 16.0);
	bool isPlayer = entity->isPlayer();

	// Step 3: Always add to global entity list first (Alpha behavior)
	// This ensures drops cannot be rejected during active gameplay
	entities.emplace(entity);
	
	if (isPlayer)
	{
		players.push_back(std::static_pointer_cast<Player>(entity));
		std::cout << "Player count: " << players.size() << '\n';
	}

	// Step 3: Only add to chunk if it's loaded (best-effort chunk bookkeeping)
	// If chunk is not loaded, skip chunk insertion but don't fail
	if (hasChunk(cx, cz))
	{
		getChunk(cx, cz)->addEntity(entity);
	}
	// If chunk not loaded, entity is still in global list and will be added to chunk
	// when it moves into a loaded chunk during tick (see Level::tick entity chunk update)

	entityAdded(entity);

	return true;
}

void Level::entityAdded(std::shared_ptr<Entity> entity)
{
	for (auto &l : listeners)
		l->entityAdded(entity);
}

void Level::entityRemoved(std::shared_ptr<Entity> entity)
{
	for (auto &l : listeners)
		l->entityRemoved(entity);
}

void Level::removeEntity(std::shared_ptr<Entity> entity)
{
	if (entity->rider != nullptr)
		entity->rider->ride(nullptr);
	if (entity->riding != nullptr)
		entity->ride(nullptr);
	entity->remove();
	if (entity->isPlayer())
	{
		// newb12: Remove player from players list (Level.java:953-955)
		std::shared_ptr<Player> player = std::static_pointer_cast<Player>(entity);
		players.erase(std::remove(players.begin(), players.end(), player), players.end());
	}
}

void Level::removeEntityImmediately(std::shared_ptr<Entity> entity)
{
	entity->remove();
	if (entity->isPlayer())
	{
		// newb12: Remove player from players list (Level.java:960-962)
		std::shared_ptr<Player> player = std::static_pointer_cast<Player>(entity);
		players.erase(std::remove(players.begin(), players.end(), player), players.end());
	}
	int_t cx = entity->xChunk;
	int_t cz = entity->zChunk;
	if (entity->inChunk && hasChunk(cx, cz))
		getChunk(cx, cz)->removeEntity(entity);
	entities.erase(entity);
	entityRemoved(entity);
}

void Level::addListener(LevelListener &listener)
{
	listeners.emplace(&listener);
}

void Level::removeListener(LevelListener &listener)
{
	listeners.erase(&listener);
}

const std::vector<AABB *> &Level::getCubes(Entity &entity, AABB &bb)
{
	boxes.clear();

	int_t x0 = Mth::floor(bb.x0);
	int_t x1 = Mth::floor(bb.x1 + 1.0);
	int_t y0 = Mth::floor(bb.y0);
	int_t y1 = Mth::floor(bb.y1 + 1.0);
	int_t z0 = Mth::floor(bb.z0);
	int_t z1 = Mth::floor(bb.z1 + 1.0);

	// Beta: Level.getCubes() - iterate x and z first, then y from var5 - 1 to var6 (Level.java:991-1000)
	for (int_t x = x0; x < x1; x++)
	{
		for (int_t z = z0; z < z1; z++)
		{
			// Beta: Check if chunk exists (Level.java:993)
			if (hasChunkAt(x, 64, z))
			{
				// Beta: for (int var11 = var5 - 1; var11 < var6; var11++) (Level.java:994)
				// Check one block below for step-up mechanics (fences, stairs, etc.)
				// Ensure y doesn't go below 0 or above DEPTH-1 to avoid array bounds errors
				int_t yStart = (y0 - 1 >= 0) ? (y0 - 1) : 0;
				int_t yEnd = (y1 < DEPTH) ? y1 : DEPTH;
				for (int_t y = yStart; y < yEnd; y++)
				{
					int_t tileId = getTile(x, y, z);
					// Beta: Check bounds before accessing Tile::tiles array (0-255)
					if (tileId >= 0 && tileId < 256)
					{
						Tile *tile = Tile::tiles[tileId];
						if (tile != nullptr)
							tile->addAABBs(*this, x, y, z, bb, boxes);
					}
				}
			}
		}
	}

	double skin = 0.25;
	const auto &overlapEntities = getEntities(&entity, *bb.grow(skin, skin, skin));

	for (auto &other : overlapEntities)
	{
		AABB *ebb = other->getCollideBox();
		if (ebb != nullptr && ebb->intersects(bb))
			boxes.push_back(ebb);

		ebb = entity.getCollideAgainstBox(*other);
		if (ebb != nullptr && ebb->intersects(bb))
			boxes.push_back(ebb);
	}

	return boxes;
}

int_t Level::getSkyDarken(float a)
{
	float tod = getTimeOfDay(a);
	float curve = 1.0f - (Mth::cos(tod * Mth::PI * 2.0f) * 2.0f + 0.5f);
	if (curve < 0.0f) curve = 0.0f;
	if (curve > 1.0f) curve = 1.0f;
	return static_cast<int_t>(curve * 11.0f);
}

static void HSBtoRGB(float hue, float saturation, float brightness, float &r, float &g, float &b)
{
	if (saturation == 0)
	{
		// Achromatic (grey)
		r = g = b = brightness;
	}
	else
	{
		hue /= (60.0f / 360.0f);
		int i = static_cast<int>(hue);
		float f = hue - i;
		float p = brightness * (1.0f - saturation);
		float q = brightness * (1.0f - saturation * f);
		float t = brightness * (1.0f - saturation * (1.0f - f));

		switch (i) {
			case 0:
				r = brightness;
				g = t;
				b = p;
				break;
			case 1:
				r = q;
				g = brightness;
				b = p;
				break;
			case 2:
				r = p;
				g = brightness;
				b = t;
				break;
			case 3:
				r = p;
				g = q;
				b = brightness;
				break;
			case 4:
				r = t;
				g = p;
				b = brightness;
				break;
			default:
				r = brightness;
				g = p;
				b = q;
				break;
		}
	}
}

Vec3 *Level::getSkyColor(Entity &entity, float a)
{
	float tod = getTimeOfDay(a);
	float curve = Mth::cos(tod * Mth::PI * 2.0f) * 2.0f + 0.5f;
	if (curve < 0.0f) curve = 0.0f;
	if (curve > 1.0f) curve = 1.0f;

	int_t x = Mth::floor(entity.x);
	int_t z = Mth::floor(entity.z);

	float temperature = getBiomeSource().getTemperature(x, z);
	
	float r = 0.0f;
	float g = 0.0f;
	float b = 1.0f;

	temperature /= 3.0f;
	if (temperature < -1.0f) temperature = -1.0f;
	if (temperature > 1.0f) temperature = 1.0f;

	HSBtoRGB(0.6222222f - temperature * 0.05f, 0.5f + temperature * 0.1f, 1.0f, r, g, b);

	r *= curve;
	g *= curve;
	b *= curve;

	return Vec3::newTemp(r, g, b);
}

float Level::getTimeOfDay(float a)
{
	return dimension->getTimeOfDay(time, a);
}

float Level::getSunAngle(float a)
{
	float tod = getTimeOfDay(a);
	return tod * Mth::PI * 2.0f;
}

Vec3 *Level::getCloudColor(float a)
{
	float tod = getTimeOfDay(a);
	float curve = Mth::cos(tod * Mth::PI * 2.0f) * 2.0f + 0.5f;
	if (curve < 0.0f) curve = 0.0f;
	if (curve > 1.0f) curve = 1.0f;

	float r = ((cloudColor >> 16) & 0xFF) / 255.0f;
	float g = ((cloudColor >> 8) & 0xFF) / 255.0f;
	float b = (cloudColor & 0xFF) / 255.0f;

	r *= curve * 0.9f + 0.1f;
	g *= curve * 0.9f + 0.1f;
	b *= curve * 0.85f + 0.15f;

	return Vec3::newTemp(r, g, b);
}

Vec3 *Level::getFogColor(float a)
{
	float tod = getTimeOfDay(a);
	return dimension->getFogColor(tod, a);
}

int_t Level::getTopSolidBlock(int_t x, int_t z)
{
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);

	int_t y;
	for (y = DEPTH - 1; getMaterial(x, y, z).blocksMotion() && y > 0; y--);

	x &= 0xF;
	z &= 0xF;

	while (y > 0)
	{
		int_t tile = chunk->getTile(x, y, z);
		if (tile == 0 || (!(Tile::tiles[tile])->material.blocksMotion() && !(Tile::tiles[tile])->material.isLiquid()))
		{
			y--;
			continue;
		}
		return y + 1;
	}
	return -1;
}

int_t Level::getLightDepth(int_t x, int_t z)
{
	return getChunkAt(x, z)->getHeightmap(x & 0xF, z & 0xF);
}

float Level::getStarBrightness(float a)
{
	float tod = getTimeOfDay(a);
	float curve = 1.0f - (Mth::cos(tod * Mth::PI * 2.0f) * 2.0f + 0.75f);
	if (curve < 0.0f) curve = 0.0f;
	if (curve > 1.0f) curve = 1.0f;
	return curve * curve * 0.5f;
}

// Alpha: World.scheduleBlockUpdate() (World.java:1117-1138)
void Level::scheduleBlockUpdate(int_t x, int_t y, int_t z, int_t blockID)
{
	// Alpha: If instaTick (field_4214_a), tick immediately instead of scheduling
	if (instaTick)
	{
		// Immediate tick: verify chunks exist and block matches, then tick
		byte_t rad = 8;
		if (hasChunksAt(x - rad, y - rad, z - rad, x + rad, y + rad, z + rad))
		{
			int_t currentBlockID = getTile(x, y, z);
			if (currentBlockID == blockID && currentBlockID > 0 && Tile::tiles[currentBlockID] != nullptr)
			{
				Tile::tiles[currentBlockID]->tick(*this, x, y, z, random);
			}
		}
		return;
	}

	// Normal scheduling: create entry and add to queue
	NextTickListEntry entry(x, y, z, blockID);
	
	// Alpha: Set scheduledTime = worldTime + Block.tickRate() (World.java:1129)
	if (blockID > 0 && Tile::tiles[blockID] != nullptr)
	{
		int_t tickRate = Tile::tiles[blockID]->getTickDelay();
		entry.scheduledTime = time + tickRate;
	}

	// Alpha: Check chunks exist before scheduling (World.java:1127)
	byte_t rad = 8;
	if (!hasChunksAt(x - rad, y - rad, z - rad, x + rad, y + rad, z + rad))
		return;

	// Alpha: Only add if not already in set (World.java:1132-1135)
	if (scheduledTickSet.find(entry) == scheduledTickSet.end())
	{
		scheduledTickSet.insert(entry);
		scheduledTickTreeSet.insert(entry);
		
		#ifdef DEBUG_FLUID_TICKS
		std::cout << "Scheduled block tick: (" << x << "," << y << "," << z << ") blockID=" << blockID 
		          << " at time=" << entry.scheduledTime << " (delay=" << entry.scheduledTime - time << ")\n";
		#endif
	}
}

void Level::addToTickNextTick(int_t x, int_t y, int_t z, int_t delay)
{
	// Legacy method - for now, just schedule with the delay as blockID (caller should pass blockID)
	// TODO: This method signature is ambiguous - check if it's actually used
	scheduleBlockUpdate(x, y, z, delay);
}

void Level::tickEntities()
{
	// Remove entities queued to remove
	for (auto &entity : entitiesToRemove)
		entities.erase(entity);

	for (auto &entity : entitiesToRemove)
	{
		int_t xc = entity->xChunk;
		int_t zc = entity->zChunk;
		if (entity->inChunk && hasChunk(xc, zc))
			getChunk(xc, zc)->removeEntity(entity);
	}

	for (auto &entity : entitiesToRemove)
		entityRemoved(entity);

	entitiesToRemove.clear();

	// Tick all entities
	for (auto it = entities.begin(); it != entities.end();)
	{
		std::shared_ptr<Entity> entity = *it;

		if (entity->riding != nullptr)
		{
			if (entity->riding->removed || entity->riding->rider != entity)
			{
				entity->riding->rider = nullptr;
				entity->riding = nullptr;
			}
			else
			{
				it++;
				continue;
			}
		}

		if (!entity->removed)
			tick(entity);

		if (entity->removed)
		{
			int_t xc = entity->xChunk;
			int_t zc = entity->zChunk;
			if (entity->inChunk && hasChunk(xc, zc))
				getChunk(xc, zc)->removeEntity(entity);
			
			it = entities.erase(it);
			entityRemoved(entity);
			continue;
		}

		it++;
		continue;
	}

	// Tick tile entities
	for (auto &tileEntity : tileEntityList)
		tileEntity->tick();
}

void Level::tick(std::shared_ptr<Entity> entity)
{
	tick(entity, true);
}

void Level::tick(std::shared_ptr<Entity> entity, bool ai)
{
	int_t xt = Mth::floor(entity->x);
	int_t zt = Mth::floor(entity->z);

	byte_t rad = 32;
	if (ai && !hasChunksAt(xt - rad, 0, zt - rad, xt + rad, DEPTH, zt + rad))
		return;

	// Interpolation setup
	entity->xOld = entity->x;
	entity->yOld = entity->y;
	entity->zOld = entity->z;
	entity->yRotO = entity->yRot;
	entity->xRotO = entity->xRot;

	if (ai && entity->inChunk)
	{
		if (entity->riding != nullptr)
			entity->rideTick();
		else
			entity->tick();
	}
	else
	{
		// TODO REMOVE
		entity->tick();
	}

	// Check if any invalid operations occured
	if (std::isnan(entity->x) || std::isinf(entity->x))
		entity->x = entity->xOld;
	if (std::isnan(entity->y) || std::isinf(entity->y))
		entity->y = entity->yOld;
	if (std::isnan(entity->z) || std::isinf(entity->z))
		entity->z = entity->zOld;
	if (std::isnan(entity->yRot) || std::isinf(entity->yRot))
		entity->yRot = entity->yRotO;
	if (std::isnan(entity->xRot) || std::isinf(entity->xRot))
		entity->xRot = entity->xRotO;

	// Update chunk
	int_t xc = Mth::floor(entity->x / 16.0);
	int_t yc = Mth::floor(entity->y / 16.0);
	int_t zc = Mth::floor(entity->z / 16.0);

	if (!entity->inChunk || entity->xChunk != xc || entity->yChunk != yc || entity->zChunk != zc)
	{
		if (entity->inChunk && hasChunk(entity->xChunk, entity->zChunk))
			getChunk(entity->xChunk, entity->zChunk)->removeEntity(entity);

		if (hasChunk(xc, zc))
		{
			entity->inChunk = true;
			getChunk(xc, zc)->addEntity(entity);
		}
		else
		{
			entity->inChunk = false;
		}
	}

	// Beta 1.2: Tick rider - matches newb12 Level.java:1265-1272 exactly
	if (ai && entity->inChunk && entity->rider != nullptr)
	{
		// Beta: if (!var1.rider.removed && var1.rider.riding == var1) { this.tick(var1.rider); } else { ... } (Level.java:1266-1271)
		if (!entity->rider->removed && entity->rider->riding == entity)
		{
			tick(entity->rider);  // Beta: this.tick(var1.rider) (Level.java:1267)
		}
		else
		{
			// Beta: var1.rider.riding = null; var1.rider = null; (Level.java:1269-1270)
			entity->rider->riding = nullptr;
			entity->rider = nullptr;
		}
	}
}

bool Level::isUnobstructed(AABB &bb)
{
	auto &overlapEntities = getEntities(nullptr, bb);
	for (auto &entity : overlapEntities)
	{
		if (!entity->removed && entity->blocksBuilding)
			return false;
	}
	return true;
}

bool Level::containsAnyLiquid(AABB &bb)
{
	// Beta: Level.containsAnyLiquid() - checks if AABB contains any liquid material (Level.java:1289-1318)
	int_t x0 = Mth::floor(bb.x0);
	int_t x1 = Mth::floor(bb.x1 + 1.0);
	int_t y0 = Mth::floor(bb.y0);
	int_t y1 = Mth::floor(bb.y1 + 1.0);
	int_t z0 = Mth::floor(bb.z0);
	int_t z1 = Mth::floor(bb.z1 + 1.0);
	
	// Beta: Handle negative coordinates (Level.java:1296-1306)
	if (bb.x0 < 0.0)
		x0--;
	if (bb.y0 < 0.0)
		y0--;
	if (bb.z0 < 0.0)
		z0--;
	
	for (int_t x = x0; x < x1; x++)
	{
		for (int_t y = y0; y < y1; y++)
		{
			for (int_t z = z0; z < z1; z++)
			{
				Tile *tile = Tile::tiles[getTile(x, y, z)];
				if (tile != nullptr && tile->material.isLiquid())
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

// Beta: Level.checkAndHandleWater() - checks for water material and applies flow physics (Level.java:1345-1383)
bool Level::checkAndHandleWater(AABB &bb, const Material &material, Entity *entity)
{
	int_t x0 = Mth::floor(bb.x0);
	int_t x1 = Mth::floor(bb.x1 + 1.0);
	int_t y0 = Mth::floor(bb.y0);
	int_t y1 = Mth::floor(bb.y1 + 1.0);
	int_t z0 = Mth::floor(bb.z0);
	int_t z1 = Mth::floor(bb.z1 + 1.0);
	
	if (!hasChunksAt(x0, y0, z0, x1, y1, z1))
		return false;
	
	bool found = false;
	Vec3 *motion = Vec3::newTemp(0.0, 0.0, 0.0);
	
	for (int_t x = x0; x < x1; x++)
	{
		for (int_t y = y0; y < y1; y++)
		{
			for (int_t z = z0; z < z1; z++)
			{
				Tile *tile = Tile::tiles[getTile(x, y, z)];
				if (tile != nullptr && &tile->material == &material)
				{
					double waterHeight = y + 1 - FluidTile::getHeight(getData(x, y, z));
					if (y1 >= waterHeight)
					{
						found = true;
						// Beta: Call handleEntityInside to accumulate flow vector (Level.java:1366)
						FluidTile *fluidTile = dynamic_cast<FluidTile*>(tile);
						if (fluidTile != nullptr)
						{
							fluidTile->handleEntityInside(*this, x, y, z, entity, *motion);
						}
					}
				}
			}
		}
	}
	
	// Beta: Apply flow vector to entity motion (Level.java:1373-1379)
	if (motion->length() > 0.0)
	{
		Vec3 *normalized = motion->normalize();
		double flowStrength = 0.004;
		if (entity != nullptr)
		{
			entity->xd = entity->xd + normalized->x * flowStrength;
			entity->yd = entity->yd + normalized->y * flowStrength;
			entity->zd = entity->zd + normalized->z * flowStrength;
		}
	}
	
	return found;
}

// Beta: containsFireTile() - checks if AABB contains fire, lava, or calmLava tiles (Level.java:1322-1341)
bool Level::containsFireTile(AABB &bb)
{
	int_t x0 = Mth::floor(bb.x0);
	int_t x1 = Mth::floor(bb.x1 + 1.0);
	int_t y0 = Mth::floor(bb.y0);
	int_t y1 = Mth::floor(bb.y1 + 1.0);
	int_t z0 = Mth::floor(bb.z0);
	int_t z1 = Mth::floor(bb.z1 + 1.0);
	
	// Beta: Check if chunks exist (Level.java:1329)
	if (hasChunksAt(x0, y0, z0, x1, y1, z1))
	{
		// Beta: Iterate through all blocks in AABB (Level.java:1330-1338)
		for (int_t x = x0; x < x1; x++)
		{
			for (int_t y = y0; y < y1; y++)
			{
				for (int_t z = z0; z < z1; z++)
				{
					int_t tileId = getTile(x, y, z);
					// Beta: Check if tile is fire, lava, or calmLava (Level.java:1334)
					if (tileId == Tile::fire.id || tileId == Tile::lava.id || tileId == Tile::calmLava.id)
					{
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

void Level::extinguishFire(int_t x, int_t y, int_t z, Facing f)
{
	if (f == Facing::DOWN)
		y--;
	if (f == Facing::UP)
		y++;
	if (f == Facing::NORTH)
		z--;
	if (f == Facing::SOUTH)
		z++;
	if (f == Facing::WEST)
		x--;
	if (f == Facing::EAST)
		x++;

	// TODO
}

jstring Level::gatherStats()
{
	return u"All: " + String::toString((int)entities.size());
}

jstring Level::gatherChunkSourceStats()
{
	return chunkSource->gatherStats();
}

std::shared_ptr<TileEntity> Level::getTileEntity(int_t x, int_t y, int_t z)
{
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	return (chunk != nullptr) ? chunk->getTileEntity(x & 0xF, y, z & 0xF) : nullptr;
}

void Level::setTileEntity(int_t x, int_t y, int_t z, std::shared_ptr<TileEntity> tileEntity)
{
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	if (chunk != nullptr)
		chunk->setTileEntity(x & 0xF, y, z & 0xF, tileEntity);
}

void Level::removeTileEntity(int_t x, int_t y, int_t z)
{
	std::shared_ptr<LevelChunk> chunk = getChunk(x >> 4, z >> 4);
	if (chunk != nullptr)
		chunk->removeTileEntity(x & 0xF, y, z & 0xF);
}

bool Level::isSolidTile(int_t x, int_t y, int_t z)
{
	Tile *tile = Tile::tiles[getTile(x, y, z)];
	return (tile == nullptr) ? false : tile->isSolidRender();
}
void Level::forceSave(std::shared_ptr<ProgressListener> progressRenderer)
{
	save(true, progressRenderer);
}

int_t Level::getLightsToUpdate()
{
	return lightUpdates.size();
}

bool Level::updateLights()
{
	if (maxRecurse >= 50)
		return false;
	maxRecurse++;

	int_t limit = 500;
	while (!lightUpdates.empty())
	{
		if (--limit <= 0)
		{
			maxRecurse--;
			return true;
		}

		LightUpdate update = lightUpdates.back();
		lightUpdates.pop_back();

		update.update(*this);
	}

	maxRecurse--;
	return false;
}

void Level::updateLight(int_t layer, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	updateLight(layer, x0, y0, z0, x1, y1, z1, true);
}

void Level::updateLight(int_t layer, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1, bool checkExpansion)
{
	if (dimension->hasCeiling && layer == LightLayer::Sky)
		return;

	if (++maxLoop == 50)
	{
		maxLoop--;
		return;
	}

	int_t xm = (x1 + x0) / 2;
	int_t zm = (z1 + z0) / 2;
	if (!hasChunkAt(xm, 64, zm))
	{
		maxLoop--;
		return;
	}

	if (getChunkAt(xm, zm)->isEmpty())
	{
		maxLoop--;
		return;
	}

	int_t updates = lightUpdates.size();
	if (checkExpansion)
	{
		// Check if this light update can be handled by another nearby one
		int_t checkLimit = 5;
		if (checkLimit > updates)
			checkLimit = updates;

		for (int_t i = 0; i < checkLimit; i++)
		{
			LightUpdate &other = lightUpdates[lightUpdates.size() - i - 1];
			if (other.layer == layer && other.expandToContain(x0, y0, z0, x1, y1, z1))
			{
				maxLoop--;
				return;
			}
		}
	}

	// Create a new light update
	lightUpdates.emplace_back(layer, x0, y0, z0, x1, y1, z1);

	int_t limit = 1000000;
	if (lightUpdates.size() > limit)
	{
		std::cout << "More than " << limit << " updates, aborting lighting updates\n";
		lightUpdates.clear();
	}

	maxLoop--;
}

void Level::updateSkyBrightness()
{
	int_t newDarken = getSkyDarken(1.0f);
	if (newDarken != skyDarken)
		skyDarken = newDarken;
}

void Level::setSpawnSettings(bool spawnEnemies, bool spawnFriendlies)
{
	this->spawnEnemies = spawnEnemies;
	this->spawnFriendlies = spawnFriendlies;
}

void Level::tick()
{
	// newb12: MobSpawner.tick() - spawn entities
	// Reference: newb12/net/minecraft/world/level/Level.java:1632
	MobSpawner::tick(*this, spawnEnemies, spawnFriendlies);

	chunkSource->tick();

	int_t newDarken = getSkyDarken(1.0f);
	if (newDarken != skyDarken)
	{
		skyDarken = newDarken;
		for (auto &l : listeners)
			l->skyColorChanged();
	}

	time++;
	if (time % saveInterval == 0)
		save(false, nullptr);

	tickPendingTicks(false);
	tickTiles();
}

void Level::tickTiles()
{
	// newb12: Level.tickTiles() - random tile ticking and ambient sounds
	// Reference: newb12/net/minecraft/world/level/Level.java:1652-1706
	chunksToPoll.clear();

	for (size_t i = 0; i < players.size(); i++)
	{
		std::shared_ptr<Player> player = players[i];
		int_t chunkX = Mth::floor(player->x / 16.0);
		int_t chunkZ = Mth::floor(player->z / 16.0);
		byte_t radius = 9;

		for (int_t dx = -radius; dx <= radius; dx++)
		{
			for (int_t dz = -radius; dz <= radius; dz++)
			{
				chunksToPoll.insert(ChunkPos(dx + chunkX, dz + chunkZ));
			}
		}
	}

	if (delayUntilNextMoodSound > 0)
	{
		delayUntilNextMoodSound--;
	}

	for (const ChunkPos &chunkPos : chunksToPoll)
	{
		int_t worldX = chunkPos.x * 16;
		int_t worldZ = chunkPos.z * 16;
		std::shared_ptr<LevelChunk> chunk = getChunk(chunkPos.x, chunkPos.z);
		if (delayUntilNextMoodSound == 0)
		{
			randValue = randValue * 3 + addend;
			int_t randResult = randValue >> 2;
			int_t localX = randResult & 15;
			int_t localZ = (randResult >> 8) & 15;
			int_t y = (randResult >> 16) & 127;
			int_t tileId = chunk->getTile(localX, y, localZ);
			int_t blockX = localX + worldX;
			int_t blockZ = localZ + worldZ;
			if (tileId == 0 && getRawBrightness(blockX, y, blockZ) <= random.nextInt(8) && getBrightness(LightLayer::Sky, blockX, y, blockZ) <= 0)
			{
				std::shared_ptr<Player> nearestPlayer = getNearestPlayer(blockX + 0.5, y + 0.5, blockZ + 0.5, 8.0);
				if (nearestPlayer != nullptr && nearestPlayer->distanceToSqr(blockX + 0.5, y + 0.5, blockZ + 0.5) > 4.0)
				{
					playSound(blockX + 0.5, y + 0.5, blockZ + 0.5, u"ambient.cave.cave", 0.7f, 0.8f + random.nextFloat() * 0.2f);
					delayUntilNextMoodSound = random.nextInt(12000) + 6000;
				}
			}
		}

		for (int_t i = 0; i < 80; i++)
		{
			randValue = randValue * 3 + addend;
			int_t randResult = randValue >> 2;
			int_t localX = randResult & 15;
			int_t localZ = (randResult >> 8) & 15;
			int_t y = (randResult >> 16) & 127;
			ubyte_t tileId = chunk->blocks[(localX << 11) | (localZ << 7) | y];
			if (Tile::shouldTick[tileId])
			{
				Tile::tiles[tileId]->tick(*this, localX + worldX, y, localZ + worldZ, random);
			}
		}
	}
}

// Alpha: World.func_700_a() / tickPendingTicks (World.java:1705-1736)
bool Level::tickPendingTicks(bool unknown)
{
	int_t tickCount = scheduledTickTreeSet.size();
	
	// Alpha: Verify TreeSet and HashSet are in sync (World.java:1710-1712)
	if (tickCount != scheduledTickSet.size())
	{
		// Out of sync - clear and rebuild (shouldn't happen, but safety check)
		scheduledTickTreeSet.clear();
		scheduledTickSet.clear();
		return false;
	}

	// Alpha: Process up to 1000 ticks per call (World.java:1713-1715)
	if (tickCount > 1000)
		tickCount = 1000;

	int_t processedCount = 0;
	
	#ifdef DEBUG_FLUID_TICKS
	int_t fluidUpdateCount = 0;
	#endif

	for (int_t i = 0; i < tickCount && !scheduledTickTreeSet.empty(); ++i)
	{
		// Alpha: Get first entry (earliest scheduled time) (World.java:1718)
		NextTickListEntry entry = *scheduledTickTreeSet.begin();
		
		// Alpha: If not forcing (unknown=false) and scheduledTime > worldTime, stop (World.java:1719-1721)
		if (!unknown && entry.scheduledTime > time)
			break;

		// Remove from both sets
		scheduledTickTreeSet.erase(scheduledTickTreeSet.begin());
		scheduledTickSet.erase(entry);

		// Alpha: Verify chunks exist before ticking (World.java:1726)
		byte_t rad = 8;
		if (!hasChunksAt(entry.xCoord - rad, entry.yCoord - rad, entry.zCoord - rad,
		                  entry.xCoord + rad, entry.yCoord + rad, entry.zCoord + rad))
			continue;

		// Alpha: Verify block ID still matches before ticking (World.java:1727-1730)
		int_t currentBlockID = getTile(entry.xCoord, entry.yCoord, entry.zCoord);
		if (currentBlockID == entry.blockID && currentBlockID > 0 && Tile::tiles[currentBlockID] != nullptr)
		{
			Tile::tiles[currentBlockID]->tick(*this, entry.xCoord, entry.yCoord, entry.zCoord, random);
			processedCount++;
			
			#ifdef DEBUG_FLUID_TICKS
			// Count fluid updates (water=8,9 or lava=10,11)
			if (currentBlockID == 8 || currentBlockID == 9 || currentBlockID == 10 || currentBlockID == 11)
				fluidUpdateCount++;
			#endif
		}
	}

	#ifdef DEBUG_FLUID_TICKS
	if (fluidUpdateCount > 0 || processedCount > 0)
	{
		std::cout << "Tick processed: " << processedCount << " updates (" << fluidUpdateCount << " fluids) "
		          << "remaining: " << scheduledTickTreeSet.size() << "\n";
	}
	#endif

	// Alpha: Return true if more ticks pending (World.java:1734)
	return !scheduledTickTreeSet.empty();
}

void Level::animateTick(int_t x, int_t y, int_t z)
{
	// newb12: Level.animateTick() - random tile animation
	// Reference: newb12/net/minecraft/world/level/Level.java:1738-1751
	byte_t radius = 16;
	Random localRandom;

	for (int_t i = 0; i < 1000; i++)
	{
		int_t blockX = x + random.nextInt(radius) - random.nextInt(radius);
		int_t blockY = y + random.nextInt(radius) - random.nextInt(radius);
		int_t blockZ = z + random.nextInt(radius) - random.nextInt(radius);
		int_t tileId = getTile(blockX, blockY, blockZ);
		if (tileId > 0)
		{
			Tile::tiles[tileId]->animateTick(*this, blockX, blockY, blockZ, localRandom);
		}
	}
}

const std::vector<std::shared_ptr<Entity>> &Level::getEntities(Entity *ignore, AABB &aabb)
{
	es.clear();

	int_t x0 = Mth::floor((aabb.x0 - 2.0) / 16.0);
	int_t x1 = Mth::floor((aabb.x1 + 2.0) / 16.0);
	int_t z0 = Mth::floor((aabb.z0 - 2.0) / 16.0);
	int_t z1 = Mth::floor((aabb.z1 + 2.0) / 16.0);

	for (int_t cx = x0; cx <= x1; cx++)
		for (int_t cz = z0; cz <= z1; cz++)
			if (hasChunk(cx, cz))  // Beta: Check if chunk exists before accessing (Level.java:1762)
				getChunk(cx, cz)->getEntities(ignore, aabb, es);

	return es;
}

const std::vector<std::shared_ptr<Entity>> &Level::getEntitiesOfCondition(bool (*condition)(Entity &), AABB &aabb)
{
	es.clear();

	int_t x0 = Mth::floor((aabb.x0 - 2.0) / 16.0);
	int_t x1 = Mth::floor((aabb.x1 + 2.0) / 16.0);
	int_t z0 = Mth::floor((aabb.z0 - 2.0) / 16.0);
	int_t z1 = Mth::floor((aabb.z1 + 2.0) / 16.0);

	for (int_t cx = x0; cx <= x1; cx++)
		for (int_t cz = z0; cz <= z1; cz++)
			getChunk(cx, cz)->getEntitiesOfCondition(condition, aabb, es);

	return es;
}

const std::unordered_set<std::shared_ptr<Entity>> &Level::getAllEntities()
{
	return entities;
}

void Level::tileEntityChanged(int_t x, int_t y, int_t z, std::shared_ptr<TileEntity> tileEntity)
{
	if (hasChunkAt(x, y, z))
		getChunkAt(x, z)->markUnsaved();
	for (auto &l : listeners)
		l->tileEntityChanged(x, y, z, tileEntity);
}

int_t Level::countConditionOf(bool (*condition)(Entity &))
{
	int_t count = 0;
	for (auto &i : entities)
		if (condition(*i))
			count++;
	return count;
}

void Level::addEntities(const std::unordered_set<std::shared_ptr<Entity>> &entities)
{
	this->entities.insert(entities.begin(), entities.end());
	for (auto &i : entities)
		entityAdded(i);
}

void Level::removeEntities(const std::unordered_set<std::shared_ptr<Entity>> &entities)
{
	entitiesToRemove.insert(entities.begin(), entities.end());
}

void Level::disconnect()
{
	// Base implementation - no-op for single-player
	// MultiPlayerLevel overrides this to send disconnect packet
}

void Level::checkSession()
{
	std::unique_ptr<File> session_lock(File::open(*dir, u"session.lock"));
	std::unique_ptr<std::istream> is(session_lock->toStreamIn());
	if (!is)
		throw std::runtime_error("Failed to check session lock, aborting");

	if (IOUtil::readLong(*is) != sessionId)
		throw std::runtime_error("The save is being accessed from another location, aborting");
}

// Beta: playSound() methods - forward to listeners (Level.java:880-889)
void Level::playSound(Entity *entity, const jstring &name, float volume, float pitch)
{
	// Beta: Forward to all listeners at entity position (Level.java:880-883)
	for (LevelListener *listener : listeners)
	{
		listener->playSound(name, entity->x, entity->y - entity->heightOffset, entity->z, volume, pitch);
	}
}

void Level::playSound(double x, double y, double z, const jstring &name, float volume, float pitch)
{
	// Beta: Forward to all listeners at specified position (Level.java:886-889)
	for (LevelListener *listener : listeners)
	{
		listener->playSound(name, x, y, z, volume, pitch);
	}
}

// Beta: addParticle() method - forward to listeners (Level.java:891-894)
void Level::addParticle(const jstring &name, double x, double y, double z, double xa, double ya, double za)
{
	// Beta: Forward to all listeners (Level.java:891-894)
	for (LevelListener *listener : listeners)
	{
		listener->addParticle(name, x, y, z, xa, ya, za);
	}
}

// Beta: playStreamingMusic() method - forward to listeners (Level.java:892-895)
void Level::playStreamingMusic(const jstring &name, int_t x, int_t y, int_t z)
{
	// Beta: Forward to all listeners (Level.java:892-895)
	for (LevelListener *listener : listeners)
	{
		listener->playStreamingMusic(name, x, y, z);
	}
}

void Level::setTime(long_t time)
{
	this->time = time;
}

void Level::ensureAdded(std::shared_ptr<Entity> entity)
{
	int_t x = Mth::floor(entity->x / 16.0);
	int_t z = Mth::floor(entity->z / 16.0);
	byte_t rad = 2;

	for (int_t cx = x - rad; cx <= x + rad; cx++)
		for (int_t cz = z - rad; cz <= z + rad; cz++)
			getChunk(cx, cz);

	for (auto &i : entities)
		if (i == entity)
			return;
	entities.emplace(entity);
}

bool Level::mayInteract(std::shared_ptr<Player> player, int_t x, int_t y, int_t z)
{
	return true;
}

void Level::broadcastEntityEvent(std::shared_ptr<Entity> entity, byte_t event)
{

}

std::shared_ptr<ChunkSource> Level::getChunkSource()
{
	return chunkSource;
}

// newb12: Level.getNearestPlayer() - finds nearest player within distance
// Reference: newb12/net/minecraft/world/level/Level.java:1931-1945
std::shared_ptr<Player> Level::getNearestPlayer(double x, double y, double z, double distance)
{
	double closestDistSqr = -1.0;
	std::shared_ptr<Player> closest = nullptr;

	for (auto &player : players)
	{
		double distSqr = player->distanceToSqr(x, y, z);
		if ((distance < 0.0 || distSqr < distance * distance) && (closestDistSqr == -1.0 || distSqr < closestDistSqr))
		{
			closestDistSqr = distSqr;
			closest = player;
		}
	}

	return closest;
}

// newb12: Level.getNearestPlayer() - helper that takes Entity
// Reference: newb12/net/minecraft/world/level/Level.java:1927-1929
std::shared_ptr<Player> Level::getNearestPlayer(Entity &entity, double distance)
{
	return getNearestPlayer(entity.x, entity.y, entity.z, distance);
}

// newb12: Level.findPath() - pathfinding
// Reference: newb12/net/minecraft/world/level/Level.java:1855-1881
std::unique_ptr<pathfinder::Path> Level::findPath(Entity &entity, Entity &target, float distance)
{
	if (pathFinder == nullptr)
	{
		pathFinder = std::make_unique<PathFinder>(this);
	}
	return pathFinder->findPath(entity, target, distance);
}

std::unique_ptr<pathfinder::Path> Level::findPath(Entity &entity, int_t x, int_t y, int_t z, float distance)
{
	if (pathFinder == nullptr)
	{
		pathFinder = std::make_unique<PathFinder>(this);
	}
	return pathFinder->findPath(entity, x, y, z, distance);
}

// newb12: Helper to count entities by MobCategory (replaces Java's countInstanceOf(getBaseClass()))
// Reference: newb12/net/minecraft/world/level/MobSpawner.java:50
int_t Level::countInstanceOf(MobCategory category) const
{
	switch (category)
	{
	case MobCategory::monster:
		return countInstanceOf<Monster>();
	case MobCategory::creature:
		return countInstanceOf<Animal>();
	case MobCategory::waterCreature:
		// WaterAnimal not yet implemented
		return 0;
	default:
		return 0;
	}
}

std::shared_ptr<Explosion> Level::explode(Entity *source, double x, double y, double z, float radius)
{
	return explode(source, x, y, z, radius, false);
}

std::shared_ptr<Explosion> Level::explode(Entity *source, double x, double y, double z, float radius, bool fire)
{
	std::shared_ptr<Explosion> explosion = Util::make_shared<Explosion>(*this, source, x, y, z, radius);
	explosion->fire = fire;
	explosion->explode();
	explosion->addParticles();
	return explosion;
}

double Level::getSeenPercent(Vec3 &from, AABB &to)
{
	// Beta: Level.getSeenPercent() - calculates visibility percentage (Level.java:1562-1580)
	// Samples points across the AABB and checks line of sight from 'from' to each point
	double var3 = 1.0 / ((to.x1 - to.x0) * 2.0 + 1.0);  // Beta: double var3 = 1.0D / ((var2.x1 - var2.x0) * 2.0D + 1.0D) (Level.java:1563)
	double var5 = 1.0 / ((to.y1 - to.y0) * 2.0 + 1.0);  // Beta: double var5 = 1.0D / ((var2.y1 - var2.y0) * 2.0D + 1.0D) (Level.java:1564)
	double var7 = 1.0 / ((to.z1 - to.z0) * 2.0 + 1.0);  // Beta: double var7 = 1.0D / ((var2.z1 - var2.z0) * 2.0D + 1.0D) (Level.java:1565)
	int_t var9 = 0;  // Beta: int var9 = 0 (Level.java:1566) - visible count
	int_t var10 = 0;  // Beta: int var10 = 0 (Level.java:1567) - total count
	
	// Beta: Sample points across the AABB volume (Level.java:1569-1582)
	for (float var11 = 0.0f; var11 <= 1.0f; var11 = (float)((double)var11 + var3))  // Beta: for(float var11 = 0.0F; var11 <= 1.0F; var11 = (float)((double)var11 + var3)) (Level.java:1569)
	{
		for (float var12 = 0.0f; var12 <= 1.0f; var12 = (float)((double)var12 + var5))  // Beta: for(float var12 = 0.0F; var12 <= 1.0F; var12 = (float)((double)var12 + var5)) (Level.java:1570)
		{
			for (float var13 = 0.0f; var13 <= 1.0f; var13 = (float)((double)var13 + var7))  // Beta: for(float var13 = 0.0F; var13 <= 1.0F; var13 = (float)((double)var13 + var7)) (Level.java:1571)
			{
				// Beta: Calculate sample point within AABB (Level.java:1572-1574)
				double var14 = to.x0 + (to.x1 - to.x0) * (double)var11;  // Beta: double var14 = var2.x0 + (var2.x1 - var2.x0) * (double)var11 (Level.java:1572)
				double var16 = to.y0 + (to.y1 - to.y0) * (double)var12;  // Beta: double var16 = var2.y0 + (var2.y1 - var2.y0) * (double)var12 (Level.java:1573)
				double var18 = to.z0 + (to.z1 - to.z0) * (double)var13;  // Beta: double var18 = var2.z0 + (var2.z1 - var2.z0) * (double)var13 (Level.java:1574)
				
				// Beta: Check line of sight - clip returns null if no hit (clear visibility) (Level.java:1575-1577)
				// Java: this.clip(Vec3.newTemp(var14, var16, var18), var1) - checks from samplePoint to from
				// If clip returns null (no hit), the path is clear (visible)
				Vec3 samplePoint(var14, var16, var18);  // Beta: Vec3.newTemp(var14, var16, var18) (Level.java:1575)
				HitResult hit = clip(samplePoint, from);  // Beta: this.clip(Vec3.newTemp(var14, var16, var18), var1) (Level.java:1575)
				if (hit.type == HitResult::Type::NONE)  // Beta: if(this.clip(...) == null) (Level.java:1575)
				{
					var9++;  // Beta: ++var9 (Level.java:1576)
				}
				
				var10++;  // Beta: ++var10 (Level.java:1579)
			}
		}
	}
	
	return (double)var9 / (double)var10;  // Beta: return (float)var9 / (float)var10 (Level.java:1584)
}
