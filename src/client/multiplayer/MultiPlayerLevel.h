#pragma once

#include "world/level/Level.h"
#include "util/IntHashMap.h"
#include "java/File.h"
#include <memory>
#include <unordered_set>
#include <list>

class NetClientHandler;

// Alpha 1.2.6: WorldClient - client-side multiplayer world implementation
// Note: In Beta 1.2 this class is called MultiPlayerLevel, but in Alpha 1.2.6 it's WorldClient
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/WorldClient.java
class MultiPlayerLevel : public Level  // TODO: Consider renaming to WorldClient to match Alpha 1.2.6
{
private:
	static constexpr int_t TICKS_BEFORE_RESET = 80;
	
	struct ResetInfo
	{
		int_t x;
		int_t y;
		int_t z;
		int_t ticks;
		int_t tile;
		int_t data;
		
		ResetInfo(int_t x, int_t y, int_t z, int_t tile, int_t data)
			: x(x), y(y), z(z), ticks(TICKS_BEFORE_RESET), tile(tile), data(data)
		{
		}
	};
	
	std::list<ResetInfo> updatesToReset;
	NetClientHandler* connection;
	class MultiPlayerChunkCache* chunkCache;
	bool isValid;  // Flag to track if this MultiPlayerLevel is still valid (not moved/destroyed)
	IntHashMap<std::shared_ptr<Entity>> entitiesById;
	std::unordered_set<std::shared_ptr<Entity>> forced;
	std::unordered_set<std::shared_ptr<Entity>> reEntries;
	int_t keepAliveCounter;  // Counter for sending keep-alive packets

public:
	MultiPlayerLevel(NetClientHandler* connection, long_t seed, int_t dimension);

	void tick();  // Not virtual in Level, but we override the behavior
	
	void clearResetRegion(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);
	
	// Beta: Get connection for sending packets (needed for TextEditScreen)
	NetClientHandler* getConnection() { return connection; }
	
protected:
	ChunkSource* createChunkSource(std::shared_ptr<File> dir);  // Not virtual in Level
	
public:
	void validateSpawn();  // Not virtual in Level
	
protected:
	void tickTiles();  // Not virtual in Level
	
public:
	void addToTickNextTick(int_t x, int_t y, int_t z, int_t delay);  // Not virtual in Level
	bool tickPendingTicks(bool unknown);  // Not virtual in Level
	
	void setChunkVisible(int_t x, int_t z, bool visible);
	
	bool addEntity(std::shared_ptr<Entity> entity);  // Not virtual in Level
	void removeEntity(std::shared_ptr<Entity> entity);  // Not virtual in Level
	
protected:
	void entityAdded(std::shared_ptr<Entity> entity);  // Not virtual in Level
	void entityRemoved(std::shared_ptr<Entity> entity);  // Not virtual in Level
	
public:
	void putEntity(int_t id, std::shared_ptr<Entity> entity);
	std::shared_ptr<Entity> getEntity(int_t id);
	std::shared_ptr<Entity> removeEntity(int_t id);
	
	bool setDataNoUpdate(int_t x, int_t y, int_t z, int_t data);  // Not virtual in Level
	bool setTileAndDataNoUpdate(int_t x, int_t y, int_t z, int_t tile, int_t data);  // Not virtual in Level
	bool setTileNoUpdate(int_t x, int_t y, int_t z, int_t tile);  // Not virtual in Level
	
	// Alpha 1.2.6: WorldClient overrides setBlockAndMetadata/setBlock/setBlockMetadata to add to delayed reset list
	// These are NOT virtual in Level, but WorldClient overrides them (shadowing in Java)
	// In C++, we need to add these methods that call the base and add to updatesToReset
	bool setTile(int_t x, int_t y, int_t z, int_t tile);  // Override to add to delayed reset list
	bool setTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t data);  // Override to add to delayed reset list
	bool setData(int_t x, int_t y, int_t z, int_t data);  // Override to add to delayed reset list
	
	bool doSetTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t data);
	
	void disconnect();
	
	// Decode and set chunk data from packet
	void setChunkData(int_t x, int_t y, int_t z, int_t xSize, int_t ySize, int_t zSize, const byte_t* data);
	
	// Initialize position tracking variables to match player's current position
	// Should be called after player is positioned (e.g., in handleSpawnPosition)
	void initializePositionTracking();
	
private:
	// Alpha 1.2.6: Send player position updates (matches EntityClientPlayerMP.sendPosition)
	void sendPlayerPosition();
	
	// Track last position/rotation for delta calculation
	double xLast;
	double yLast1;  // boundingBox.minY
	double yLast2;  // y position
	double zLast;
	float yRotLast;
	float xRotLast;
	bool lastOnGround;
	int_t noSendTime;  // Counter to limit sending when not moving
};
