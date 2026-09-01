#pragma once

// libnx services the game depends on. Owned by the core library rather than
// main so that shared code can ask whether networking actually came up.
namespace switchruntime
{

// Called from userAppInit, before the C++ static constructors run, because the
// game has static objects that read resources out of romfs.
void initialize();
void shutdown();

// Brings up the BSD sockets service. Idempotent, and the only place that does
// so, which keeps isNetworkAvailable an honest answer. initialize() calls it.
bool initializeNetwork();

// False when the console has no usable network. libnx's BSD sockets abort
// instead of failing gracefully when the service did not initialise, so every
// caller has to check this before touching a socket.
bool isNetworkAvailable();

// Writes the process's used/total memory to the log. Switch homebrew launched
// as an applet gets a far smaller heap than a full title, so an allocation
// failure needs the figures to be interpretable.
void reportMemory(const char *stage);
}
