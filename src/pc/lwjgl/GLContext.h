#pragma once

#include <string>

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
	bool operator[](const std::string &capability) const;
};

}

// Context functions
// Idempotent. Explicit backend selection must happen before the first call.
void instantiate();
const detail::GLCapabilities &getCapabilities();

}
}
