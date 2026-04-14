#include "client/renderer/ChunkMeshWorker.h"

ChunkMeshWorker::~ChunkMeshWorker()
{
	stop();
}

void ChunkMeshWorker::start()
{
	if (running.load()) return;
	running.store(true);

	workerThread = std::thread([this]() { workerLoop(); });
}

void ChunkMeshWorker::stop()
{
	if (!running.load()) return;

	running.store(false);
	queueCV.notify_all();

	if (workerThread.joinable())
		workerThread.join();

	clearAll();
}

void ChunkMeshWorker::workerLoop()
{
	// Create a thread-local offline Tesselator (LCE: Tesselator::CreateNewThreadStorage(1024*1024))
	Tesselator workerTess(WORKER_TESS_SIZE, true);
	Tesselator::setThreadInstance(&workerTess);

	while (running.load())
	{
		std::unique_ptr<ChunkBuildTask> task;

		{
			std::unique_lock<std::mutex> lock(queueMutex);
			queueCV.wait(lock, [this]() {
				return !pendingTasks.empty() || !running.load();
			});

			if (!running.load()) break;
			if (pendingTasks.empty()) continue;

			task = std::move(pendingTasks.front());
			pendingTasks.pop_front();
		}

		// Build the mesh (pure CPU, no GL calls)
		task->chunk->buildMesh(*task->snapshot, workerTess, task->result);

		// Free the snapshot immediately - we're done reading from it
		task->snapshot.reset();

		// Push to completed queue
		{
			std::lock_guard<std::mutex> lock(resultMutex);
			completedTasks.push_back(std::move(task));
		}
	}
}

bool ChunkMeshWorker::submitTask(std::unique_ptr<ChunkBuildTask> task)
{
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		if (queuedTaskCount.load() >= MAX_QUEUED_TASKS)
			return false;

		pendingTasks.push_back(std::move(task));
		queuedTaskCount.fetch_add(1);
	}
	queueCV.notify_one();
	return true;
}

int_t ChunkMeshWorker::drainCompleted(int_t maxUploads)
{
	int_t uploaded = 0;

	while (uploaded < maxUploads)
	{
		std::unique_ptr<ChunkBuildTask> task;

		{
			std::lock_guard<std::mutex> lock(resultMutex);
			if (completedTasks.empty()) break;
			task = std::move(completedTasks.front());
			completedTasks.pop_front();
		}

		// If the chunk went dirty again while we were building, discard and re-queue
		if (task->chunk->dirty)
		{
			task->chunk->inFlight = false;
			queuedTaskCount.fetch_sub(1);
			continue;
		}

		// Upload to GPU (main thread only)
		task->chunk->uploadMesh(task->result);
		queuedTaskCount.fetch_sub(1);
		uploaded++;
	}

	return uploaded;
}

void ChunkMeshWorker::clearAll()
{
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		for (auto &task : pendingTasks)
			task->chunk->inFlight = false;
		queuedTaskCount.fetch_sub(pendingTasks.size());
		pendingTasks.clear();
	}
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		for (auto &task : completedTasks)
			task->chunk->inFlight = false;
		queuedTaskCount.fetch_sub(completedTasks.size());
		completedTasks.clear();
	}
}

bool ChunkMeshWorker::hasPendingWork() const
{
	return queuedTaskCount.load() != 0;
}
