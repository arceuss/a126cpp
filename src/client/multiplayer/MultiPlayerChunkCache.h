#pragma once

#include "world/level/chunk/ChunkSource.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/chunk/EmptyLevelChunk.h"
#include <memory>
#include <unordered_map>
#include <vector>

class Level;

// Beta 1.2: MultiPlayerChunkCache - chunk cache for multiplayer worlds
// Reference: beta1.2/minecraft/src/net/minecraft/client/multiplayer/MultiPlayerChunkCache.java
class MultiPlayerChunkCache : public ChunkSource
{
private:
	struct ChunkPos
	{
		int_t x;
		int_t z;
		
		ChunkPos(int_t x, int_t z) : x(x), z(z) {}
		
		bool operator==(const ChunkPos& other) const
		{
			return x == other.x && z == other.z;
		}
	};
	
	struct ChunkPosHash
	{
		std::size_t operator()(const ChunkPos& pos) const
		{
			return (static_cast<std::size_t>(pos.x) << 16) ^ static_cast<std::size_t>(pos.z);
		}
	};

	std::shared_ptr<LevelChunk> emptyChunk;
	std::unordered_map<ChunkPos, std::shared_ptr<LevelChunk>, ChunkPosHash> loadedChunks;
	std::vector<std::shared_ptr<LevelChunk>> loadedChunkList;
	Level& level;

public:
	MultiPlayerChunkCache(Level& level);

	bool hasChunk(int_t x, int_t z) override;
	std::shared_ptr<LevelChunk> getChunk(int_t x, int_t z) override;
	void postProcess(ChunkSource& parent, int_t x, int_t z) override;
	bool save(bool force, std::shared_ptr<ProgressListener> progressListener) override;
	bool tick() override;
	bool shouldSave() override;
	jstring gatherStats() override;

	// Beta 1.2: drop() and create() methods for chunk management
	void drop(int_t x, int_t z);
	std::shared_ptr<LevelChunk> create(int_t x, int_t z);
};
