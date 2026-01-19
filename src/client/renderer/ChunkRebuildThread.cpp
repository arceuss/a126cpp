#include "client/renderer/ChunkRebuildThread.h"
#include "client/renderer/Chunk.h"
#include <algorithm>
#include <thread>
#include <memory>

ChunkRebuildThreadPool::ChunkRebuildThreadPool(size_t numThreads)
{
	// Use hardware concurrency minus 1 (leave one core for main thread)
	// But ensure at least 1 thread
	if (numThreads == 0)
	{
		unsigned int hwConcurrency = std::thread::hardware_concurrency();
		size_t defaultThreads = (hwConcurrency > 1) ? static_cast<size_t>(hwConcurrency - 1) : 1;
		numThreads = defaultThreads;
	}
	
	// Cap at 8 threads to avoid overhead
	numThreads = std::min(numThreads, static_cast<size_t>(8));
	
	// Create worker threads
	for (size_t i = 0; i < numThreads; i++)
	{
		threads.emplace_back(&ChunkRebuildThreadPool::workerThread, this);
	}
}

ChunkRebuildThreadPool::~ChunkRebuildThreadPool()
{
	shutdown();
}

void ChunkRebuildThreadPool::shutdown()
{
	{
		std::unique_lock<std::mutex> lock(workMutex);
		stopFlag = true;
	}
	workCondition.notify_all();
	
	for (auto& thread : threads)
	{
		if (thread.joinable())
			thread.join();
	}
	threads.clear();
}

void ChunkRebuildThreadPool::submit(std::vector<std::shared_ptr<Chunk>> chunks)
{
	if (chunks.empty()) return;
	
	{
		std::unique_lock<std::mutex> lock(workMutex);
		for (auto& chunk : chunks)
		{
			if (chunk && chunk->dirty)
			{
				workQueue.push(chunk);
			}
		}
	}
	
	if (!chunks.empty())
	{
		workCondition.notify_all();
	}
}

std::vector<std::shared_ptr<Chunk>> ChunkRebuildThreadPool::getCompleted()
{
	std::unique_lock<std::mutex> lock(completedMutex);
	
	if (completedQueue.empty())
		return {};
	
	std::vector<std::shared_ptr<Chunk>> result;
	result.swap(completedQueue);
	return result;
}

void ChunkRebuildThreadPool::waitForCompletion()
{
	std::unique_lock<std::mutex> lock(workMutex);
	workCondition.wait(lock, [this] {
		return workQueue.empty() && activeWorkers == 0;
	});
}

size_t ChunkRebuildThreadPool::getPendingCount() const
{
	std::unique_lock<std::mutex> lock(workMutex);
	return workQueue.size();
}

void ChunkRebuildThreadPool::workerThread()
{
	while (true)
	{
		std::shared_ptr<Chunk> chunk;
		
		{
			std::unique_lock<std::mutex> lock(workMutex);
			
			workCondition.wait(lock, [this] {
				return !workQueue.empty() || stopFlag;
			});
			
			if (stopFlag && workQueue.empty())
				break;
			
			if (!workQueue.empty())
			{
				chunk = workQueue.front();
				workQueue.pop();
				activeWorkers++;
			}
		}
		
		if (chunk)
		{
			// NOTE: Chunk rebuild uses OpenGL (VBO creation) which must be on main thread
			// This thread pool is prepared for future optimization where we separate
			// CPU geometry building from GPU VBO upload. For now, we mark chunks for
			// rebuilding and the main thread handles it.
			// 
			// Future: Build geometry data in CPU buffers here, upload to GPU on main thread
			
			// Currently, we just mark that this chunk needs rebuilding
			// The actual rebuild happens on main thread due to OpenGL requirements
			// This structure allows us to add CPU-side geometry building in the future
			
			{
				std::unique_lock<std::mutex> lock(completedMutex);
				completedQueue.push_back(chunk);
			}
			
			{
				std::unique_lock<std::mutex> lock(workMutex);
				activeWorkers--;
			}
			
			completedCondition.notify_one();
			workCondition.notify_one(); // Notify waitForCompletion
		}
	}
}
