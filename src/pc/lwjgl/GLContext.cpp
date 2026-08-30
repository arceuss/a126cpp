#include "lwjgl/GLContext.h"

#include <cstdlib>

#include "backends/Backend.h"
#include "backends/Platform/Platform.h"
#include "legacygl/Startup.h"

namespace lwjgl
{
namespace GLContext
{
namespace detail
{

bool GLCapabilities::operator[](const std::string &capability) const
{
	return renderbackend::hasCapability(capability.c_str());
}

static void shutdown()
{
	renderbackend::shutdown();
	platform::destroyWindow();
	platform::shutdown();
}

}

void instantiate()
{
	// Repeated calls retain the old make-current behavior while the linked
	// backend and LegacyGL sink themselves remain process-latched.
	platform::initialize();
	try
	{
		renderbackend::initialize();
	}
	catch (...)
	{
		renderbackend::shutdown();
		platform::destroyWindow();
		platform::shutdown();
		throw;
	}
	static const bool installed = []()
	{
		std::atexit(detail::shutdown);
		legacygl::installSelectedBackend();
		return true;
	}();
	(void)installed;
}

const detail::GLCapabilities &getCapabilities()
{
	static const detail::GLCapabilities capabilities;
	return capabilities;
}

}
}
