#pragma once

// Shared SDL2 state between platform and rendering backends.
// The SDL2 platform backend creates the window/context;
// other backends (e.g., OpenGL2 rendering) may need access.

#include "SDL.h"

#ifdef _WIN32
#undef APIENTRY
#endif

namespace SDL2_Shared
{
    SDL_Window* getWindow();
    SDL_GLContext getGLContext();
    void setWindow(SDL_Window* window);
    void setGLContext(SDL_GLContext context);
}
