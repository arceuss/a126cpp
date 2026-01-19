#pragma once

#include <stdexcept>

#include <SDL3/SDL.h>

class SDLException : public std::runtime_error
{
public:
	SDLException() : std::runtime_error(SDL_GetError())
	{
	}
};
