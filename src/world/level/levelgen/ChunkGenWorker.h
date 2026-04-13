#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <memory>
#include <unordered_set>

#include "java/Type.h"

#include "world/level/levelgen/RandomLevelSource.h"
#include "world/level/levelgen/LargeCaveFeature.h"

class Level;
class LevelChunk;

// A single chunk generation task
struct ChunkGenTask
{
	int_t chunkX = 0;
	int_t chunkZ = 0;

	// Result: filled in by worker thread
	std::shared_ptr<LevelChunk> result;
};

// Background worker thread that generates chunk terrain off the main thread.
// Follows the LCE 4J pattern: worker runs prepareHeights + buildSurfaces + caves,
// main thread inserts chunk, runs recalcHeightmap + postProcess.
class ChunkGenWorker
{
private:
	std::thread workerThread;

	std::mutex queueMutex;
	std::condition_variable queueCV;
	std::deque<std::unique_ptr<ChunkGenTask>> pendingTasks;

	std::mutex resultMutex;
	std::deque<std::unique_ptr<ChunkGenTask>> completedTasks;

	std::atomic<bool> running{false};

	// Reference to the level source for noise generators
	RandomLevelSource &source;
	Level &level;

	// Set of in-flight chunk coordinates to avoid duplicate requests
	std::mutex inflightMutex;
	std::unordered_set<long_t> inflightChunks;

	void workerLoop();

	static long_t chunkKey(int_t x, int_t z) { return (static_cast<long_t>(x) & 0xFFFFFFFF) | (static_cast<long_t>(z) << 32); }

public:
	ChunkGenWorker(RandomLevelSource &source, Level &level);
	~ChunkGenWorker();

	void start();
	void stop();

	// Main thread: request a chunk to be generated async
	// Returns false if already in-flight
	bool requestChunk(int_t x, int_t z);

	// Main thread: check if a specific chunk is ready, returns it and removes from completed
	std::shared_ptr<LevelChunk> pollCompleted(int_t x, int_t z);

	// Main thread: drain all completed chunks into a vector
	int_t drainCompleted(std::deque<std::unique_ptr<ChunkGenTask>> &out, int_t maxDrain);

	// Main thread: discard all pending and completed tasks
	void clearAll();

	bool isInFlight(int_t x, int_t z);
};
