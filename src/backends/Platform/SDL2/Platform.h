#pragma once

#include "SDL.h"

namespace platform
{
namespace sdl2
{

// SDL-specific renderer glue. Game code and fixtures use Platform.h instead.
SDL_Window *window();

}
}
