#include "client/multiplayer/MultiPlayerChunkCache.h"
#include "world/level/Level.h"
#include "world/level/chunk/EmptyLevelChunk.h"
#include "util/Memory.h"
#include "java/String.h"
#include <algorithm>
#include <cstring>

MultiPlayerChunkCache::MultiPlayerChunkCache(Level& level)
	: level(level)
{
	// Beta 1.2: Create empty chunk (32768 bytes = 16*16*128)
	emptyChunk = Util::make_shared<EmptyLevelChunk>(level, 0, 0);
}

bool MultiPlayerChunkCache::hasChunk(int_t x, int_t z)
{
	ChunkPos pos(x, z);
	return loadedChunks.find(pos) != loadedChunks.end();
}

std::shared_ptr<LevelChunk> MultiPlayerChunkCache::getChunk(int_t x, int_t z)
{
	ChunkPos pos(x, z);
	auto it = loadedChunks.find(pos);
	if (it != loadedChunks.end())
	{
		return it->second;
	}
	// Alpha 1.2.6: getChunkFromChunkCoords creates chunk if it doesn't exist
	// For multiplayer, create empty chunk that will be populated by packet data
	return create(x, z);
}

void MultiPlayerChunkCache::drop(int_t x, int_t z)
{
	std::shared_ptr<LevelChunk> chunk = getChunk(x, z);
	if (chunk != nullptr && !chunk->isEmpty())
	{
		chunk->unload();
	}
	
	ChunkPos pos(x, z);
	auto it = loadedChunks.find(pos);
	if (it != loadedChunks.end())
	{
		loadedChunkList.erase(
			std::remove(loadedChunkList.begin(), loadedChunkList.end(), it->second),
			loadedChunkList.end()
		);
		loadedChunks.erase(it);
	}
}

std::shared_ptr<LevelChunk> MultiPlayerChunkCache::create(int_t x, int_t z)
{
	ChunkPos pos(x, z);
	
	// Beta 1.2: Create chunk with 32768 bytes (16*16*128)
	std::vector<ubyte_t> bytes(32768, 0);
	std::shared_ptr<LevelChunk> chunk = Util::make_shared<LevelChunk>(level, bytes.data(), x, z);
	
	// Beta 1.2: Fill skyLight with -1 (all lit)
	std::fill(chunk->skyLight.data.begin(), chunk->skyLight.data.end(), static_cast<ubyte_t>(0xFF));
	
	loadedChunks[pos] = chunk;
	loadedChunkList.push_back(chunk);
	chunk->loaded = true;
	
	return chunk;
}

void MultiPlayerChunkCache::postProcess(ChunkSource& parent, int_t x, int_t z)
{
	// Beta 1.2: No post-processing in multiplayer
}

bool MultiPlayerChunkCache::save(bool force, std::shared_ptr<ProgressListener> progressListener)
{
	// Beta 1.2: Multiplayer chunks are not saved
	return true;
}

bool MultiPlayerChunkCache::tick()
{
	// Beta 1.2: No ticking in multiplayer chunk cache
	return false;
}

bool MultiPlayerChunkCache::shouldSave()
{
	// Beta 1.2: Multiplayer chunks should not be saved
	return false;
}

jstring MultiPlayerChunkCache::gatherStats()
{
	// Beta 1.2: Return chunk count
	return u"MultiplayerChunkCache: " + String::toString(static_cast<int_t>(loadedChunks.size()));
}
