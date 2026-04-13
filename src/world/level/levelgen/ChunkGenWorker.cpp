#include "world/level/levelgen/ChunkGenWorker.h"

#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"

ChunkGenWorker::ChunkGenWorker(RandomLevelSource &source, Level &level)
	: source(source), level(level)
{
}

ChunkGenWorker::~ChunkGenWorker()
{
	stop();
}

void ChunkGenWorker::start()
{
	if (running.load())
		return;
	running.store(true);
	workerThread = std::thread(&ChunkGenWorker::workerLoop, this);
}

void ChunkGenWorker::stop()
{
	if (!running.load())
		return;
	running.store(false);
	queueCV.notify_all();
	if (workerThread.joinable())
		workerThread.join();
}

void ChunkGenWorker::workerLoop()
{
	// Per-thread resources (4J: "moved here for thread safety")
	ChunkGenScratch scratch;
	Random rng(0);
	LargeCaveFeature caveFeature;

	while (running.load())
	{
		std::unique_ptr<ChunkGenTask> task;

		{
			std::unique_lock<std::mutex> lock(queueMutex);
			queueCV.wait(lock, [this] { return !pendingTasks.empty() || !running.load(); });

			if (!running.load() && pendingTasks.empty())
				break;

			if (pendingTasks.empty())
				continue;

			task = std::move(pendingTasks.front());
			pendingTasks.pop_front();
		}

		// Generate terrain off-thread (no Level writes, no recalcHeightmap)
		task->result = source.generateTerrain(task->chunkX, task->chunkZ, rng, scratch, caveFeature);

		// Move to completed queue
		{
			std::lock_guard<std::mutex> lock(resultMutex);
			completedTasks.push_back(std::move(task));
		}
	}
}

bool ChunkGenWorker::requestChunk(int_t x, int_t z)
{
	long_t key = chunkKey(x, z);

	{
		std::lock_guard<std::mutex> lock(inflightMutex);
		if (inflightChunks.count(key))
			return false;
		inflightChunks.insert(key);
	}

	auto task = std::unique_ptr<ChunkGenTask>(new ChunkGenTask());
	task->chunkX = x;
	task->chunkZ = z;

	{
		std::lock_guard<std::mutex> lock(queueMutex);
		pendingTasks.push_back(std::move(task));
	}
	queueCV.notify_one();

	return true;
}

std::shared_ptr<LevelChunk> ChunkGenWorker::pollCompleted(int_t x, int_t z)
{
	std::lock_guard<std::mutex> lock(resultMutex);

	for (auto it = completedTasks.begin(); it != completedTasks.end(); ++it)
	{
		if ((*it)->chunkX == x && (*it)->chunkZ == z)
		{
			auto result = std::move((*it)->result);
			completedTasks.erase(it);

			// Remove from inflight set
			{
				std::lock_guard<std::mutex> ilock(inflightMutex);
				inflightChunks.erase(chunkKey(x, z));
			}

			return result;
		}
	}

	return nullptr;
}

int_t ChunkGenWorker::drainCompleted(std::deque<std::unique_ptr<ChunkGenTask>> &out, int_t maxDrain)
{
	std::lock_guard<std::mutex> lock(resultMutex);

	int_t drained = 0;
	while (!completedTasks.empty() && drained < maxDrain)
	{
		auto task = std::move(completedTasks.front());
		completedTasks.pop_front();

		{
			std::lock_guard<std::mutex> ilock(inflightMutex);
			inflightChunks.erase(chunkKey(task->chunkX, task->chunkZ));
		}

		out.push_back(std::move(task));
		drained++;
	}

	return drained;
}

void ChunkGenWorker::clearAll()
{
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		pendingTasks.clear();
	}
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		completedTasks.clear();
	}
	{
		std::lock_guard<std::mutex> lock(inflightMutex);
		inflightChunks.clear();
	}
}

bool ChunkGenWorker::isInFlight(int_t x, int_t z)
{
	std::lock_guard<std::mutex> lock(inflightMutex);
	return inflightChunks.count(chunkKey(x, z)) > 0;
}
