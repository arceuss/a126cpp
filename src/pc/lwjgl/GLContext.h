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
// Idempotent. The backend is selected when the executable is linked.
void instantiate();
const detail::GLCapabilities &getCapabilities();

}
}
