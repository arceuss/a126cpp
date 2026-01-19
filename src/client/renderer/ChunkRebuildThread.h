#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <memory>
#include <atomic>

class Chunk;

// Thread pool for parallel chunk rebuilding
class ChunkRebuildThreadPool
{
public:
	ChunkRebuildThreadPool(size_t numThreads = 0);
	~ChunkRebuildThreadPool();

	// Submit chunks for rebuilding
	void submit(std::vector<std::shared_ptr<Chunk>> chunks);
	
	// Get completed chunks (non-blocking, returns empty if none ready)
	std::vector<std::shared_ptr<Chunk>> getCompleted();
	
	// Wait for all submitted chunks to complete
	void waitForCompletion();
	
	// Get number of pending rebuilds
	size_t getPendingCount() const;
	
	// Shutdown the thread pool
	void shutdown();

private:
	void workerThread();
	
	std::vector<std::thread> threads;
	std::queue<std::shared_ptr<Chunk>> workQueue;
	std::vector<std::shared_ptr<Chunk>> completedQueue;
	
	mutable std::mutex workMutex;
	std::mutex completedMutex;
	std::condition_variable workCondition;
	std::condition_variable completedCondition;
	
	std::atomic<bool> stopFlag{false};
	std::atomic<size_t> activeWorkers{0};
};
