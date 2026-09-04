#include "util/BackgroundTask.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace BackgroundTask
{

struct Task
{
	std::thread thread;
	std::atomic<bool> finished{false};
};

static std::mutex tasksMutex;
// unique_ptr so a Task's address stays valid when the vector grows: the running
// thread holds a raw pointer to its own entry.
static std::vector<std::unique_ptr<Task>> tasks;

void run(std::function<void()> work)
{
	std::unique_ptr<Task> task(new Task());
	Task *entry = task.get();

	entry->thread = std::thread([entry, work]() {
		work();
		entry->finished.store(true, std::memory_order_release);
	});

	std::lock_guard<std::mutex> guard(tasksMutex);
	tasks.push_back(std::move(task));
}

void reap()
{
	std::lock_guard<std::mutex> guard(tasksMutex);

	for (size_t i = 0; i < tasks.size();)
	{
		if (tasks[i]->finished.load(std::memory_order_acquire))
		{
			tasks[i]->thread.join();
			tasks.erase(tasks.begin() + static_cast<std::ptrdiff_t>(i));
		}
		else
		{
			i++;
		}
	}
}

void joinAll()
{
	// Join outside the lock: a task may spawn another task while it winds
	// down (the connection close thread starts the master thread from
	// networkShutdown), and run() needs the mutex to register it.
	for (;;)
	{
		std::unique_ptr<Task> task;
		{
			std::lock_guard<std::mutex> guard(tasksMutex);
			if (tasks.empty())
				return;
			task = std::move(tasks.front());
			tasks.erase(tasks.begin());
		}
		if (task->thread.joinable())
			task->thread.join();
	}
}

}
