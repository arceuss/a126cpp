#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <memory>

#include "client/renderer/Chunk.h"
#include "client/renderer/ChunkSnapshot.h"
#include "client/renderer/Tesselator.h"

// A single task submitted to the mesh worker
struct ChunkBuildTask
{
	std::shared_ptr<Chunk> chunk;
	std::unique_ptr<ChunkSnapshot> snapshot;
	ChunkBuildResult result;
};

// Background worker thread that builds chunk meshes off the main thread.
// Follows the LCE pattern: main thread creates snapshots and submits tasks,
// worker thread tessellates into CPU buffers, main thread uploads VBOs.
class ChunkMeshWorker
{
private:
	std::thread workerThread;

	std::mutex queueMutex;
	std::condition_variable queueCV;
	std::deque<std::unique_ptr<ChunkBuildTask>> pendingTasks;

	std::mutex resultMutex;
	std::deque<std::unique_ptr<ChunkBuildTask>> completedTasks;

	std::atomic<bool> running{false};

	// Thread-local offline tesselator (~2MB buffer, no GL calls)
	static constexpr int_t WORKER_TESS_SIZE = 0x80000; // 512K floats = 2MB

	void workerLoop();

public:
	ChunkMeshWorker() = default;
	~ChunkMeshWorker();

	void start();
	void stop();

	// Main thread: submit a chunk for async meshing
	void submitTask(std::unique_ptr<ChunkBuildTask> task);

	// Main thread: upload completed meshes to GPU (up to maxUploads)
	int_t drainCompleted(int_t maxUploads);

	// Main thread: discard all pending and completed tasks
	void clearAll();

	bool hasPendingWork() const;
};
