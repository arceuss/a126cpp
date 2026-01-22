#pragma once

#include <string>
#include <set>

#include "external/SDLCompat.h"
#include "glad/glad.h"

namespace lwjgl
{
namespace GLContext
{

// Detail implementation
namespace detail
{

// GL capabilities
struct GLCapabilities
{
private:
	std::set<std::string> caps;

public:
	void add(const std::string &cap)
	{
		caps.insert(cap);
	}

	bool operator[](const std::string &cap) const
	{
		return caps.find(cap) != caps.end();
	}
};

// Context singletons (SDL 1.2)
SDL_Surface *getScreen();

// Recreate the SDL 1.2 OpenGL video mode (note: this recreates the GL context).
// Returns false on failure.
bool setVideoMode(int width, int height, bool fullscreen, bool resizable);

}

// Context functions
void instantiate();
const detail::GLCapabilities &getCapabilities();

}
}
