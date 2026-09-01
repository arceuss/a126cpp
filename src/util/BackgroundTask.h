#pragma once

#include <functional>

// Fire-and-forget work that must not use std::thread::detach.
//
// Detaching crashes on devkitA64/libnx: the underlying thread object is
// released while the thread is still running. Threads started here stay
// joinable and are joined once they report completion, which behaves
// identically on desktop.
namespace BackgroundTask
{

// Starts work on its own thread and returns immediately.
void run(std::function<void()> work);

// Joins threads that have finished. Cheap when there is nothing to reap, so it
// is safe to call every tick.
void reap();

// Joins every outstanding thread. For shutdown.
void joinAll();

}
