#pragma once

#include "world/level/LevelSource.h"

#include <memory>
#include <unordered_set>
#include <set>

#include "nbt/Tag.h"
#include "nbt/CompoundTag.h"

#include "java/Type.h"
#include "java/String.h"
#include "java/System.h"
#include "java/File.h"
#include "java/Random.h"

#include "world/entity/Entity.h"
#include "world/entity/player/Player.h"

class Explosion;

#include "world/level/LightLayer.h"
#include "world/level/LevelListener.h"
#include "world/level/LightUpdate.h"

#include "world/level/chunk/LevelChunk.h"
#include "world/level/chunk/ChunkSource.h"
#include "world/level/biome/BiomeSource.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/Tile.h"
#include "world/level/NextTickListEntry.h"
#include "world/level/ChunkPos.h"
#include "world/level/pathfinder/Path.h"
#include "world/level/pathfinder/PathFinder.h"

#include "util/ProgressListener.h"

#include "util/Memory.h"

enum class MobCategory;

class Level : public LevelSource
{
private:
	static constexpr int_t MAX_TICK_TILES_PER_TICK = 1000;
public:
	static constexpr int_t MAX_LEVEL_SIZE = 32000000;
	static constexpr short_t DEPTH = 128;
	static constexpr short_t SEA_LEVEL = 63;
	
	bool instaTick = false;

	static constexpr int_t MAX_BRIGHTNESS = 15;
	static constexpr int_t TICKS_PER_DAY = 24000;

private:
	// Alpha: World.scheduledTickTreeSet and scheduledTickSet (World.java:22-23)
	// TreeSet for ordered processing, HashSet for duplicate detection
	std::set<NextTickListEntry> scheduledTickTreeSet;
	std::unordered_set<NextTickListEntry> scheduledTickSet;

public:
	std::vector<LightUpdate> lightUpdates;

public:
	std::unordered_set<std::shared_ptr<Entity>> entities;
	std::unordered_set<std::shared_ptr<Entity>> entitiesToRemove;

	std::unordered_set<std::shared_ptr<TileEntity>> tileEntityList;

	std::vector<std::shared_ptr<Player>> players;

	long_t time = 0;

private:
	long_t cloudColor = 0xFFFFFF;

public:
	int_t skyDarken = 0;

protected:
	int_t randValue = Random().nextInt();
	int_t addend = 1013904223;

public:
	bool noNeighborUpdate = false;

private:
	long_t sessionId = System::currentTimeMillis();

protected:
	int_t saveInterval = 40;

public:
	int_t difficulty = 0;
	
	Random random;

	int_t xSpawn = 0, ySpawn = 0, zSpawn = 0;

	bool isNew = false;

	std::shared_ptr<Dimension> dimension;

protected:
	std::unordered_set<LevelListener *> listeners;
	std::shared_ptr<ChunkSource> chunkSource;

public:
	std::shared_ptr<File> workDir, dir;

	long_t seed = 0;

private:
	std::shared_ptr<CompoundTag> loadedPlayerTag;

public:
	long_t sizeOnDisk = 0;

	jstring name;

	bool isFindingSpawn = false;

private:
	std::vector<AABB *> boxes;

	int_t maxRecurse = 0;

	bool spawnEnemies = true;
	bool spawnFriendlies = true;

public:
	static int_t maxLoop;

private:
	int_t delayUntilNextMoodSound = random.nextInt(12000);

	std::unordered_set<ChunkPos> chunksToPoll;

	std::vector<std::shared_ptr<Entity>> es;

public:
	bool isOnline = false;

	static std::shared_ptr<CompoundTag> getDataTagFor(File &workingDirectory, const jstring &name);

	static void deleteLevel(File &workingDirectory, const jstring &name);
	static void deleteRecursive(std::vector<std::unique_ptr<File>> &files);

	BiomeSource &getBiomeSource() override;

	static long_t getLevelSize(File &workingDirectory, const jstring &name);
	static long_t calcSize(std::vector<std::unique_ptr<File>> &files);

	Level(File *workingDirectory, const jstring &name);
	Level(const jstring &name, int_t dimension, long_t seed);
	Level(Level &level, int_t dimension);
	Level(File *workingDirectory, const jstring &name, long_t seed);
	Level(File *workingDirectory, const jstring &name, long_t seed, int_t dimension);

protected:
	virtual ChunkSource *createChunkSource(std::shared_ptr<File> dir);

public:
	void validateSpawn();
	int_t getTopTile(int_t x, int_t z);

	void clearLoadedPlayerData();
	void loadPlayer(std::shared_ptr<Player> player);

	void save(bool force, std::shared_ptr<ProgressListener> progressRenderer);
	void saveLevelData();

	bool pauseSave(int_t saveStep);

	int_t getTile(int_t x, int_t y, int_t z) override;

	bool isEmptyTile(int_t x, int_t y, int_t z);

	bool hasChunkAt(int_t x, int_t y, int_t z);
	bool hasChunksAt(int_t x, int_t y, int_t z, int_t d);
	bool hasChunksAt(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);

private:
	bool hasChunk(int_t xc, int_t zc);

public:
	std::shared_ptr<LevelChunk> getChunkAt(int_t x, int_t z);
	std::shared_ptr<LevelChunk> getChunk(int_t xc, int_t zc);

	bool setTileAndDataNoUpdate(int_t x, int_t y, int_t z, int_t tile, int_t data);
	bool setTileNoUpdate(int_t x, int_t y, int_t z, int_t tile);

	const Material &getMaterial(int_t x, int_t y, int_t z) override;

	int_t getData(int_t x, int_t y, int_t z) override;
	void setData(int_t x, int_t y, int_t z, int_t data);
	bool setDataNoUpdate(int_t x, int_t y, int_t z, int_t data);

	bool setTile(int_t x, int_t y, int_t z, int_t tile);
	bool setTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t data);

	// Beta: Level.mayPlace() - checks if block can be placed (Level.java:1833-1848)
	bool mayPlace(int_t tileId, int_t x, int_t y, int_t z, bool flag);

	void sendTileUpdated(int_t x, int_t y, int_t z);
	void tileUpdated(int_t x, int_t y, int_t z, int_t tile);

	void lightColumnChanged(int_t x, int_t z, int_t y0, int_t y1);
	void setTileDirty(int_t x, int_t y, int_t z);
	void setTilesDirty(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);

	void swap(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);
	void updateNeighborsAt(int_t x, int_t y, int_t z, int_t tile);
	void neighborChanged(int_t x, int_t y, int_t z, int_t tile);

	// Beta: Level.getSignal() - gets redstone signal strength (Level.java:1904-1912)
	bool getSignal(int_t x, int_t y, int_t z, int_t facing);

	// Beta: Level.getDirectSignal() - gets direct redstone signal (Level.java:1885-1888)
	bool getDirectSignal(int_t x, int_t y, int_t z, int_t facing);

	// Beta: Level.hasNeighborSignal() - checks if any neighbor has signal (Level.java:1913-1925)
	bool hasNeighborSignal(int_t x, int_t y, int_t z);

	bool canSeeSky(int_t x, int_t y, int_t z);

	int_t getRawBrightness(int_t x, int_t y, int_t z);
	int_t getRawBrightness(int_t x, int_t y, int_t z, bool neighbors);

	bool isSkyLit(int_t x, int_t y, int_t z);
	int_t getHeightmap(int_t x, int_t z);

	void updateLightIfOtherThan(int_t layer, int_t x, int_t y, int_t z, int_t brightness);

	int_t getBrightness(int_t layer, int_t x, int_t y, int_t z);
	void setBrightness(int_t layer, int_t x, int_t y, int_t z, int_t brightness);
	float getBrightness(int_t x, int_t y, int_t z) override;

	bool isDay();

	HitResult clip(Vec3 &from, Vec3 &to);
	HitResult clip(Vec3 &from, Vec3 &to, bool canPickLiquid);

	bool addEntity(std::shared_ptr<Entity> entity);

protected:
	void entityAdded(std::shared_ptr<Entity> entity);
	void entityRemoved(std::shared_ptr<Entity> entity);

public:
	void removeEntity(std::shared_ptr<Entity> entity);
	void removeEntityImmediately(std::shared_ptr<Entity> entity);

	void addListener(LevelListener &listener);
	void removeListener(LevelListener &listener);

	const std::vector<AABB *> &getCubes(Entity &entity, AABB &bb);

	int_t getSkyDarken(float a);
	Vec3 *getSkyColor(Entity &entity, float a);
	float getTimeOfDay(float a);
	float getSunAngle(float a);
	Vec3 *getCloudColor(float a);
	Vec3 *getFogColor(float a);

	int_t getTopSolidBlock(int_t x, int_t z);
	int_t getLightDepth(int_t x, int_t z);

	float getStarBrightness(float a);

	// Alpha: World.scheduleBlockUpdate() (World.java:1117-1138)
	// Schedules a block update after delay ticks (uses block's tickRate() if delay not specified)
	void scheduleBlockUpdate(int_t x, int_t y, int_t z, int_t blockID);
	
	// Legacy method - delegates to scheduleBlockUpdate
	void addToTickNextTick(int_t x, int_t y, int_t z, int_t delay);

	void tickEntities();
	void tick(std::shared_ptr<Entity> entity);
	void tick(std::shared_ptr<Entity> entity, bool ai);

	bool isUnobstructed(AABB &bb);
	bool containsAnyLiquid(AABB &bb);
	bool containsFireTile(AABB &bb);

	// Beta: Level.checkAndHandleWater() - checks for water material and applies flow physics (Level.java:1345-1383)
	bool checkAndHandleWater(AABB &bb, const Material &material, Entity *entity);

	void extinguishFire(int_t x, int_t y, int_t z, Facing f);

	jstring gatherStats();
	jstring gatherChunkSourceStats();

	std::shared_ptr<TileEntity> getTileEntity(int_t x, int_t y, int_t z) override;
	void setTileEntity(int_t x, int_t y, int_t z, std::shared_ptr<TileEntity> tileEntity);
	void removeTileEntity(int_t x, int_t y, int_t z);

	bool isSolidTile(int_t x, int_t y, int_t z) override;

	void forceSave(std::shared_ptr<ProgressListener> progressRenderer);

	int_t getLightsToUpdate();

	bool updateLights();

	void updateLight(int_t layer, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);
	void updateLight(int_t layer, int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1, bool checkExpansion);
	void updateSkyBrightness();

	void setSpawnSettings(bool spawnEnemies, bool spawnFriendlies);

	virtual void tick();

protected:
	void tickTiles();

public:
	bool tickPendingTicks(bool unknown);
	void animateTick(int_t x, int_t y, int_t z);

	const std::vector<std::shared_ptr<Entity>> &getEntities(Entity *ignore, AABB &aabb);
	const std::vector<std::shared_ptr<Entity>> &getEntitiesOfCondition(bool (*condition)(Entity&), AABB &aabb);
	const std::unordered_set<std::shared_ptr<Entity>> &getAllEntities();

	void tileEntityChanged(int_t x, int_t y, int_t z, std::shared_ptr<TileEntity> tileEntity);

	int_t countConditionOf(bool (*condition)(Entity&));

	// newb12: Level.countInstanceOf() - counts entities by type
	// Reference: newb12/net/minecraft/world/level/Level.java:1803-1814
	// Note: Java uses Class.isAssignableFrom(), in C++ we use dynamic_cast with base classes
	template<typename T>
	int_t countInstanceOf() const
	{
		int_t count = 0;
		for (auto &entity : entities)
		{
			if (dynamic_cast<T*>(entity.get()) != nullptr)
				count++;
		}
		return count;
	}
	
	// Helper to count entities by MobCategory (replaces Java's countInstanceOf(getBaseClass()))
	// newb12: Used in MobSpawner as var0.countInstanceOf(var36.getBaseClass())
	// Reference: newb12/net/minecraft/world/level/MobSpawner.java:50
	int_t countInstanceOf(MobCategory category) const;

	// newb12: Level.getNearestPlayer() - finds nearest player within distance
	// Reference: newb12/net/minecraft/world/level/Level.java:1931-1945
	std::shared_ptr<Player> getNearestPlayer(double x, double y, double z, double distance);
	
	// newb12: Level.getNearestPlayer() - helper that takes Entity
	// Reference: newb12/net/minecraft/world/level/Level.java:1927-1929
	std::shared_ptr<Player> getNearestPlayer(Entity &entity, double distance);

	// newb12: Level.findPath() - pathfinding methods
	// Reference: newb12/net/minecraft/world/level/Level.java:1855-1881
	std::unique_ptr<pathfinder::Path> findPath(Entity &entity, Entity &target, float distance);
	std::unique_ptr<pathfinder::Path> findPath(Entity &entity, int_t x, int_t y, int_t z, float distance);

	void addEntities(const std::unordered_set<std::shared_ptr<Entity>> &entities);
	void removeEntities(const std::unordered_set<std::shared_ptr<Entity>> &entities);

	virtual void disconnect();  // Virtual so MultiPlayerLevel can override

	void checkSession();

	void setTime(long_t time);

	void ensureAdded(std::shared_ptr<Entity> entity);

	bool mayInteract(std::shared_ptr<Player> player, int_t x, int_t y, int_t z);

	void broadcastEntityEvent(std::shared_ptr<Entity> entity, byte_t event);

	std::shared_ptr<ChunkSource> getChunkSource();

	// Beta: playSound() methods - forward to listeners (Level.java:880-889)
	void playSound(Entity *entity, const jstring &name, float volume, float pitch);
	void playSound(double x, double y, double z, const jstring &name, float volume, float pitch);
	
	// Beta: addParticle() method - forward to listeners (Level.java:891-894)
	void addParticle(const jstring &name, double x, double y, double z, double xa, double ya, double za);
	
	// Beta: playStreamingMusic() method - forward to listeners (Level.java:892-895)
	void playStreamingMusic(const jstring &name, int_t x, int_t y, int_t z);

	// Beta: Level.explode() - creates and executes explosion (Level.java:1550-1559)
	std::shared_ptr<Explosion> explode(Entity *source, double x, double y, double z, float radius);
	std::shared_ptr<Explosion> explode(Entity *source, double x, double y, double z, float radius, bool fire);

	// Beta: Level.getSeenPercent() - calculates visibility percentage (Level.java:1562-1580)
	double getSeenPercent(Vec3 &from, AABB &to);

private:
	std::unique_ptr<PathFinder> pathFinder;
};
