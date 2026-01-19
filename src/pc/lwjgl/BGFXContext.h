#pragma once

#include <SDL3/SDL.h>

#ifdef USE_BGFX
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#endif

namespace lwjgl
{
namespace BGFXContext
{

#ifdef USE_BGFX

// Initialize bgfx with SDL3 window
// Returns true on success, false on failure
bool init(SDL_Window* window, uint32_t width, uint32_t height);

// Shutdown bgfx
void shutdown();

// Resize bgfx backbuffer
void resize(uint32_t width, uint32_t height);

// Get native window handle for bgfx
void* getNativeWindowHandle(SDL_Window* window);

// Frame rendering - call at the end of each frame
void frame();

#else

// Stub implementations when bgfx is not enabled
inline bool init(SDL_Window*, uint32_t, uint32_t) { return false; }
inline void shutdown() {}
inline void resize(uint32_t, uint32_t) {}
inline void* getNativeWindowHandle(SDL_Window*) { return nullptr; }
inline void frame() {}

#endif

}
}
