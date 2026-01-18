#include "client/multiplayer/MultiPlayerLevel.h"
#include "network/Packet255KickDisconnect.h"
#include "network/NetClientHandler.h"
#include "client/multiplayer/MultiPlayerChunkCache.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/tile/StoneSlabTile.h"
#include "world/entity/Entity.h"
#include "world/entity/player/Player.h"
#include "client/Minecraft.h"
#include "network/Packet0KeepAlive.h"
#include "network/Packet10Flying.h"
#include "network/Packet11PlayerPosition.h"
#include "network/Packet12PlayerLook.h"
#include "network/Packet13PlayerLookMove.h"
#include "network/Packet255KickDisconnect.h"
#include "util/Memory.h"
#include "java/File.h"
#include <algorithm>

MultiPlayerLevel::MultiPlayerLevel(NetClientHandler* connection, long_t seed, int_t dimension)
	: Level(u"MpServer", dimension, seed)
	, connection(connection)
	, chunkCache(nullptr)
	, keepAliveCounter(0)
{
	// Alpha 1.2.6: WorldClient constructor - sets spawn position
	// Java: this.spawnX = 8; this.spawnY = 64; this.spawnZ = 8;
	this->xSpawn = 8;
	this->ySpawn = 64;
	this->zSpawn = 8;
	this->isOnline = true;
	
	// Alpha 1.2.6: WorldClient uses ChunkProviderClient (created in func_4081_a)
	// Java: this.C = new ChunkProviderClient(this); (done in func_4081_a, called by super constructor)
	// Note: We use MultiPlayerChunkCache which is similar to ChunkProviderClient
	chunkCache = new MultiPlayerChunkCache(*this);
	this->chunkSource.reset(chunkCache);
	
	// Note: Position tracking variables are NOT part of WorldClient in Alpha 1.2.6
	// Position updates are sent by EntityClientPlayerMP::func_4056_N(), not WorldClient::tick()
	// These are kept for potential future use but are not used in WorldClient::tick()
	xLast = 0.0;
	yLast1 = 0.0;
	yLast2 = 0.0;
	zLast = 0.0;
	yRotLast = 0.0f;
	xRotLast = 0.0f;
	lastOnGround = false;
	noSendTime = 0;
}

void MultiPlayerLevel::tick()
{
	// Beta 1.2: Increment time and update sky darkness
	this->time++;
	int_t newDark = this->getSkyDarken(1.0F);
	if (newDark != this->skyDarken)
	{
		this->skyDarken = newDark;
		
		for (LevelListener* listener : listeners)
		{
			listener->skyColorChanged();
		}
	}
	
	// Beta 1.2: Process re-entries (entities that failed to add)
	for (int_t i = 0; i < 10 && !reEntries.empty(); i++)
	{
		auto it = reEntries.begin();
		std::shared_ptr<Entity> e = *it;
		if (entities.find(e) == entities.end())
		{
			this->addEntity(e);
		}
	}
	
	// Alpha 1.2.6: WorldClient.tick() only calls sendQueue.processReadPackets()
	// Keep-alive packets are sent by GuiDownloadTerrain (every 20 ticks)
	// Position updates are sent by EntityClientPlayerMP.func_4056_N() (called from onUpdate())
	// NOT sent from WorldClient::tick() - this is Beta 1.2 behavior, not Alpha 1.2.6
	if (connection != nullptr)
	{
		// Java: this.sendQueue.processReadPackets();
		connection->processReadPackets();
	}
	
	// Beta 1.2: Process delayed block resets
	for (auto it = updatesToReset.begin(); it != updatesToReset.end();)
	{
		ResetInfo& r = *it;
		if (--r.ticks == 0)
		{
			Level::setTileAndDataNoUpdate(r.x, r.y, r.z, r.tile, r.data);
			sendTileUpdated(r.x, r.y, r.z);
			it = updatesToReset.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void MultiPlayerLevel::clearResetRegion(int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
	for (auto it = updatesToReset.begin(); it != updatesToReset.end();)
	{
		const ResetInfo& r = *it;
		if (r.x >= x0 && r.y >= y0 && r.z >= z0 && r.x <= x1 && r.y <= y1 && r.z <= z1)
		{
			it = updatesToReset.erase(it);
		}
		else
		{
			++it;
		}
	}
}

ChunkSource* MultiPlayerLevel::createChunkSource(std::shared_ptr<File> dir)
{
	// Beta 1.2: Create MultiPlayerChunkCache
	chunkCache = new MultiPlayerChunkCache(*this);
	return chunkCache;
}

void MultiPlayerLevel::validateSpawn()
{
	// Beta 1.2: Set spawn to default position
	this->xSpawn = 8;
	this->ySpawn = 64;
	this->zSpawn = 8;
}

void MultiPlayerLevel::tickTiles()
{
	// Beta 1.2: No tile ticking in multiplayer
}

void MultiPlayerLevel::addToTickNextTick(int_t x, int_t y, int_t z, int_t delay)
{
	// Beta 1.2: No scheduled ticks in multiplayer
}

bool MultiPlayerLevel::tickPendingTicks(bool unknown)
{
	// Beta 1.2: No pending ticks in multiplayer
	return false;
}

void MultiPlayerLevel::setChunkVisible(int_t x, int_t z, bool visible)
{
	if (chunkCache == nullptr)
		return;
		
	if (visible)
	{
		chunkCache->create(x, z);
	}
	else
	{
		chunkCache->drop(x, z);
	}
	
	if (!visible)
	{
		// Beta 1.2: Mark tiles as dirty when chunk is unloaded
		this->setTilesDirty(x * 16, 0, z * 16, x * 16 + 15, 128, z * 16 + 15);
	}
}

bool MultiPlayerLevel::addEntity(std::shared_ptr<Entity> entity)
{
	// Beta 1.2: Track entity in forced set
	bool ok = Level::addEntity(entity);
	forced.insert(entity);
	if (!ok)
	{
		reEntries.insert(entity);
	}
	return ok;
}

void MultiPlayerLevel::removeEntity(std::shared_ptr<Entity> entity)
{
	Level::removeEntity(entity);
	forced.erase(entity);
}

void MultiPlayerLevel::entityAdded(std::shared_ptr<Entity> entity)
{
	Level::entityAdded(entity);
	reEntries.erase(entity);
}

void MultiPlayerLevel::entityRemoved(std::shared_ptr<Entity> entity)
{
	Level::entityRemoved(entity);
	if (forced.find(entity) != forced.end())
	{
		reEntries.insert(entity);
	}
}

void MultiPlayerLevel::putEntity(int_t id, std::shared_ptr<Entity> entity)
{
	// Beta 1.2: Remove old entity if exists
	std::shared_ptr<Entity> old = getEntity(id);
	if (old != nullptr)
	{
		removeEntity(old);
	}
	
	forced.insert(entity);
	entity->entityId = id;
	if (!addEntity(entity))
	{
		reEntries.insert(entity);
	}
	
	entitiesById.put(id, entity);
}

std::shared_ptr<Entity> MultiPlayerLevel::getEntity(int_t id)
{
	return entitiesById.get(id);
}

std::shared_ptr<Entity> MultiPlayerLevel::removeEntity(int_t id)
{
	std::shared_ptr<Entity> e = entitiesById.remove(id);
	if (e != nullptr)
	{
		forced.erase(e);
		removeEntity(e);
	}
	return e;
}

bool MultiPlayerLevel::setDataNoUpdate(int_t x, int_t y, int_t z, int_t data)
{
	int_t t = getTile(x, y, z);
	int_t d = getData(x, y, z);
	if (Level::setDataNoUpdate(x, y, z, data))
	{
		updatesToReset.push_back(ResetInfo(x, y, z, t, d));
		return true;
	}
	return false;
}

bool MultiPlayerLevel::setTileAndDataNoUpdate(int_t x, int_t y, int_t z, int_t tile, int_t data)
{
	int_t t = getTile(x, y, z);
	int_t d = getData(x, y, z);
	if (Level::setTileAndDataNoUpdate(x, y, z, tile, data))
	{
		updatesToReset.push_back(ResetInfo(x, y, z, t, d));
		return true;
	}
	return false;
}

bool MultiPlayerLevel::setTileNoUpdate(int_t x, int_t y, int_t z, int_t tile)
{
	int_t t = getTile(x, y, z);
	int_t d = getData(x, y, z);
	if (Level::setTileNoUpdate(x, y, z, tile))
	{
		updatesToReset.push_back(ResetInfo(x, y, z, t, d));
		return true;
	}
	return false;
}

// Alpha 1.2.6: WorldClient.setBlockAndMetadata() - adds to delayed reset list
// Java: public boolean setBlockAndMetadata(int var1, int var2, int var3, int var4, int var5) {
//     int var6 = this.getBlockId(var1, var2, var3);
//     int var7 = this.getBlockMetadata(var1, var2, var3);
//     if(super.setBlockAndMetadata(var1, var2, var3, var4, var5)) {
//         this.field_1057_z.add(new WorldBlockPositionType(this, var1, var2, var3, var6, var7));
//         return true;
//     } else {
//         return false;
//     }
// }
bool MultiPlayerLevel::setTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t data)
{
	int_t oldTile = getTile(x, y, z);
	int_t oldData = getData(x, y, z);
	if (Level::setTileAndData(x, y, z, tile, data))
	{
		updatesToReset.push_back(ResetInfo(x, y, z, oldTile, oldData));
		return true;
	}
	return false;
}

// Alpha 1.2.6: WorldClient.setBlock() - adds to delayed reset list
// Java: public boolean setBlock(int var1, int var2, int var3, int var4) {
//     int var5 = this.getBlockId(var1, var2, var3);
//     int var6 = this.getBlockMetadata(var1, var2, var3);
//     if(super.setBlock(var1, var2, var3, var4)) {
//         this.field_1057_z.add(new WorldBlockPositionType(this, var1, var2, var3, var5, var6));
//         return true;
//     } else {
//         return false;
//     }
// }
bool MultiPlayerLevel::setTile(int_t x, int_t y, int_t z, int_t tile)
{
	int_t oldTile = getTile(x, y, z);
	int_t oldData = getData(x, y, z);
	if (Level::setTile(x, y, z, tile))
	{
		updatesToReset.push_back(ResetInfo(x, y, z, oldTile, oldData));
		return true;
	}
	return false;
}

// Alpha 1.2.6: WorldClient.setBlockMetadata() - adds to delayed reset list
// Java: public boolean setBlockMetadata(int var1, int var2, int var3, int var4) {
//     int var5 = this.getBlockId(var1, var2, var3);
//     int var6 = this.getBlockMetadata(var1, var2, var3);
//     if(super.setBlockMetadata(var1, var2, var3, var4)) {
//         this.field_1057_z.add(new WorldBlockPositionType(this, var1, var2, var3, var5, var6));
//         return true;
//     } else {
//         return false;
//     }
// }
bool MultiPlayerLevel::setData(int_t x, int_t y, int_t z, int_t data)
{
	// Alpha 1.2.6: WorldClient.setBlockMetadata() - adds to delayed reset list
	// Java: public boolean setBlockMetadata(int var1, int var2, int var3, int var4) {
	//     int var5 = this.getBlockId(var1, var2, var3);
	//     int var6 = this.getBlockMetadata(var1, var2, var3);
	//     if(super.setBlockMetadata(var1, var2, var3, var4)) {
	//         this.field_1057_z.add(new WorldBlockPositionType(this, var1, var2, var3, var5, var6));
	//         return true;
	//     } else {
	//         return false;
	//     }
	// }
	// Note: In Java, World.setBlockMetadata() returns boolean, but in C++ Level::setData() returns void
	// We use setDataNoUpdate() which returns bool, then call tileUpdated() if successful
	int_t oldTile = getTile(x, y, z);
	int_t oldData = getData(x, y, z);
	if (Level::setDataNoUpdate(x, y, z, data))
	{
		tileUpdated(x, y, z, getTile(x, y, z));
		updatesToReset.push_back(ResetInfo(x, y, z, oldTile, oldData));
		return true;
	}
	return false;
}

bool MultiPlayerLevel::doSetTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t data)
{
	// Alpha 1.2.6: WorldClient.func_714_c() - clears reset region then sets block
	// Java: public boolean func_714_c(int var1, int var2, int var3, int var4, int var5) {
	//     this.func_711_c(var1, var2, var3, var1, var2, var3);
	//     if(super.setBlockAndMetadata(var1, var2, var3, var4, var5)) {
	//         this.notifyBlockChange(var1, var2, var3, var4);
	//         return true;
	//     } else {
	//         return false;
	//     }
	// }
	clearResetRegion(x, y, z, x, y, z);
	if (Level::setTileAndData(x, y, z, tile, data))
	{
		tileUpdated(x, y, z, tile);
		
		// Alpha 1.2.6: Mark tiles dirty for this block
		setTilesDirty(x, y, z, x, y, z);
		
		// Alpha 1.2.6: Update lighting for neighboring steps and farmland
		// These blocks check their neighbors' stored lighting values, so we need to
		// ensure those stored values are updated when neighbors change
		int_t neighbors[6][3] = {
			{x - 1, y, z}, {x + 1, y, z},
			{x, y - 1, z}, {x, y + 1, z},
			{x, y, z - 1}, {x, y, z + 1}
		};
		
		for (int i = 0; i < 6; i++)
		{
			int_t nx = neighbors[i][0];
			int_t ny = neighbors[i][1];
			int_t nz = neighbors[i][2];
			int_t neighborTile = getTile(nx, ny, nz);
			
			// If neighbor is a step or farmland, update its lighting to recalculate stored values
			if (neighborTile == Tile::stoneSlabHalf.id || neighborTile == Tile::farmland.id)
			{
				// Trigger lighting update to recalculate and store lighting values
				updateLight(LightLayer::Sky, nx, ny, nz, nx, ny, nz);
				updateLight(LightLayer::Block, nx, ny, nz, nx, ny, nz);
				// Mark dirty for rendering
				setTilesDirty(nx, ny, nz, nx, ny, nz);
			}
		}
		
		return true;
	}
	return false;
}

void MultiPlayerLevel::disconnect()
{
	// Alpha 1.2.6: WorldClient.sendQuittingDisconnectingPacket()
	// Java: this.sendQueue.func_28117_a(new Packet255KickDisconnect("Quitting"));
	// Just queues the packet - doesn't immediately disconnect
	// The connection will close when level is set to null
	if (connection != nullptr && !connection->isDisconnected())
	{
		// Queue Packet255 - matches Alpha 1.2.6 exactly
		// Java: this.sendQueue.func_28117_a(new Packet255KickDisconnect("Quitting"));
		// func_28117_a queues the packet AND wakes the writer thread
		connection->func_28117_a(new Packet255KickDisconnect(u"Quitting"));
	}
}

void MultiPlayerLevel::setChunkData(int_t x, int_t y, int_t z, int_t xSize, int_t ySize, int_t zSize, const byte_t* data)
{
	// Alpha 1.2.6: EXACT implementation of World.func_693_a
	// World.func_693_a(int var1, int var2, int var3, int var4, int var5, int var6, byte[] var7)
	int_t var8 = x >> 4;  // chunkXStart
	int_t var9 = z >> 4;  // chunkZStart
	int_t var10 = (x + xSize - 1) >> 4;  // chunkXEnd
	int_t var11 = (z + zSize - 1) >> 4;  // chunkZEnd
	int_t var12 = 0;  // offset - CRITICAL: Must be passed between chunks!
	int_t var13 = y;  // yStart
	int_t var14 = y + ySize;  // yEnd
	
	if (var13 < 0) var13 = 0;
	if (var14 > Level::DEPTH) var14 = Level::DEPTH;
	
	// Alpha 1.2.6: Loop through chunks in EXACT order
	for (int_t var15 = var8; var15 <= var10; ++var15)  // chunkX
	{
		int_t var16 = x - var15 * 16;  // xStart (local to chunk)
		int_t var17 = x + xSize - var15 * 16;  // xEnd (local to chunk)
		if (var16 < 0) var16 = 0;
		if (var17 > 16) var17 = 16;
		
		for (int_t var18 = var9; var18 <= var11; ++var18)  // chunkZ
		{
			int_t var19 = z - var18 * 16;  // zStart (local to chunk)
			int_t var20 = z + zSize - var18 * 16;  // zEnd (local to chunk)
			if (var19 < 0) var19 = 0;
			if (var20 > 16) var20 = 16;
			
			// Alpha 1.2.6: getChunkFromChunkCoords gets or creates chunk
			std::shared_ptr<LevelChunk> chunk = getChunk(var15, var18);
			if (chunk == nullptr)
				continue;
			
			// Check if we got an empty chunk (EmptyLevelChunk returns true for isEmpty())
			if (chunk->isEmpty())
			{
				if (chunkCache != nullptr)
				{
					chunkCache->drop(var15, var18);
					chunk = chunkCache->create(var15, var18);
				}
				else
				{
					continue;
				}
			}
			
			// Alpha 1.2.6: chunk.func_1004_a(data, xStart, yStart, zStart, xEnd, yEnd, zEnd, offset)
			// CRITICAL: offset is passed IN and returned OUT, then used for NEXT chunk!
			// This is the exact signature: func_1004_a(byte[] var1, int var2, int var3, int var4, int var5, int var6, int var7, int var8)
			var12 = chunk->setBlocksAndDataFromPacket(
				const_cast<byte_t*>(data), var16, var13, var19, var17, var14, var20, var12
			);
			
			// Alpha 1.2.6: func_701_b updates render bounds
			setTilesDirty(var15 * 16 + var16, var13, var18 * 16 + var19,
			              var15 * 16 + var17, var14, var18 * 16 + var20);
			
			// After loading chunk data, update lighting for steps/farmland in this region
			// These blocks check their neighbors' stored lighting values, so we need to
			// ensure those stored values are recalculated after chunks load
			for (int_t lx = var16; lx < var17; ++lx)
			{
				for (int_t ly = var13; ly < var14; ++ly)
				{
					for (int_t lz = var19; lz < var20; ++lz)
					{
						int_t wx = var15 * 16 + lx;
						int_t wz = var18 * 16 + lz;
						int_t tile = chunk->getTile(lx, ly, lz);
						
						// If this block is a step or farmland, update its lighting
						if (tile == Tile::stoneSlabHalf.id || tile == Tile::farmland.id)
						{
							updateLight(LightLayer::Sky, wx, ly, wz, wx, ly, wz);
							updateLight(LightLayer::Block, wx, ly, wz, wx, ly, wz);
						}
					}
				}
			}
		}
	}
}

void MultiPlayerLevel::sendPlayerPosition()
{
	// Alpha 1.2.6: Send player position updates (EntityClientPlayerMP.sendPosition)
	// This is critical - the server expects regular position updates or it will timeout
	if (connection == nullptr)
		return;
	
	// Get player from connection's Minecraft instance
	Minecraft* mc = connection->getMinecraft();
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	Player* player = mc->player.get();
	
	// Calculate deltas
	double xdd = player->x - xLast;
	double ydd1 = player->bb.y0 - yLast1;
	double ydd2 = player->y - yLast2;
	double zdd = player->z - zLast;
	double rydd = (double)(player->yRot - yRotLast);
	double rxdd = (double)(player->xRot - xRotLast);
	
	bool move = (ydd1 != 0.0 || ydd2 != 0.0 || xdd != 0.0 || zdd != 0.0);
	bool rot = (rydd != 0.0 || rxdd != 0.0);
	
	// Alpha 1.2.6: Send appropriate packet based on movement/rotation
	// EntityClientPlayerMP.java:67-91
	if (move && rot)
	{
		// Send position + rotation
		connection->addToSendQueue(new Packet13PlayerLookMove(
			player->x, player->bb.y0, player->y, player->z,
			player->yRot, player->xRot, player->onGround
		));
		noSendTime = 0;
	}
	else if (move)
	{
		// Send position only
		connection->addToSendQueue(new Packet11PlayerPosition(
			player->x, player->bb.y0, player->y, player->z, player->onGround
		));
		noSendTime = 0;
	}
	else if (rot)
	{
		// Send rotation only
		connection->addToSendQueue(new Packet12PlayerLook(
			player->yRot, player->xRot, player->onGround
		));
		noSendTime = 0;
	}
	else
	{
		// Send minimal flying packet (just onGround flag)
		connection->addToSendQueue(new Packet10Flying(player->onGround));
		if (lastOnGround == player->onGround && noSendTime <= 20)
		{
			++noSendTime;
		}
		else
		{
			noSendTime = 0;
		}
	}
	
	// Update last values
	lastOnGround = player->onGround;
	if (move)
	{
		xLast = player->x;
		yLast1 = player->bb.y0;
		yLast2 = player->y;
		zLast = player->z;
	}
	if (rot)
	{
		yRotLast = player->yRot;
		xRotLast = player->xRot;
	}
}

void MultiPlayerLevel::initializePositionTracking()
{
	// Initialize position tracking to current player position
	// This prevents sending incorrect deltas on first tick after spawn
	if (connection == nullptr)
		return;
	
	Minecraft* mc = connection->getMinecraft();
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	Player* player = mc->player.get();
	xLast = player->x;
	yLast1 = player->bb.y0;
	yLast2 = player->y;
	zLast = player->z;
	yRotLast = player->yRot;
	xRotLast = player->xRot;
	lastOnGround = player->onGround;
	noSendTime = 0;
}
