#pragma once

#include <string>
#include <set>

// Forward declarations for platform types
// The actual types are defined by the platform backend (e.g., SDL2)
struct SDL_Window;
typedef void* SDL_GLContext;

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

// Context singletons - implemented by platform backend
SDL_Window *getWindow();
SDL_GLContext getGLContext();

}

// Context functions
void instantiate();
const detail::GLCapabilities &getCapabilities();

}
}
