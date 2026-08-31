#include "backends/Backend.h"

#include <cstring>

namespace renderbackend
{

struct DispatcherState
{
	const Backend *selected;
	bool locked;
};

static DispatcherState &dispatcherState()
{
	static DispatcherState state = { nullptr, false };
	return state;
}

static const Backend *firstAvailableBackend()
{
#if defined(A126_HAS_BACKEND_NATIVEGL)
	return &nativeGLBackend();
#elif defined(A126_HAS_BACKEND_OPENGL21)
	return &openGL21Backend();
#elif defined(A126_HAS_BACKEND_OPENGL46)
	return &openGL46Backend();
#elif defined(A126_HAS_BACKEND_VULKAN)
	return &vulkanBackend();
#elif defined(A126_HAS_BACKEND_D3D12)
	return &d3d12Backend();
#else
	return nullptr;
#endif
}

static const Backend *findBackend(const char *cliName)
{
	if (cliName == nullptr)
		return nullptr;

#if defined(A126_HAS_BACKEND_NATIVEGL)
	if (std::strcmp(cliName, nativeGLBackend().cliName) == 0)
		return &nativeGLBackend();
#endif
#if defined(A126_HAS_BACKEND_OPENGL21)
	if (std::strcmp(cliName, openGL21Backend().cliName) == 0)
		return &openGL21Backend();
#endif
#if defined(A126_HAS_BACKEND_OPENGL46)
	if (std::strcmp(cliName, openGL46Backend().cliName) == 0)
		return &openGL46Backend();
#endif
#if defined(A126_HAS_BACKEND_VULKAN)
	if (std::strcmp(cliName, vulkanBackend().cliName) == 0)
		return &vulkanBackend();
#endif
#if defined(A126_HAS_BACKEND_D3D12)
	if (std::strcmp(cliName, d3d12Backend().cliName) == 0)
		return &d3d12Backend();
#endif

	return nullptr;
}

static const Backend *defaultBackend()
{
#ifdef A126_DEFAULT_RENDER_BACKEND
	const Backend *configured = findBackend(A126_DEFAULT_RENDER_BACKEND);
	if (configured != nullptr)
		return configured;
#endif
	return firstAvailableBackend();
}

static const Backend &activeBackend()
{
	DispatcherState &state = dispatcherState();
	if (state.selected == nullptr)
		state.selected = defaultBackend();
	return *state.selected;
}

bool select(const char *cliName)
{
	DispatcherState &state = dispatcherState();
	const Backend *backend = findBackend(cliName);
	if (backend == nullptr)
		return false;
	if (state.locked)
		return state.selected == backend;
	state.selected = backend;
	return true;
}

const Configuration &configuration()
{
	return activeBackend().configuration();
}

void initialize()
{
	DispatcherState &state = dispatcherState();
	if (state.selected == nullptr)
		state.selected = defaultBackend();
	state.locked = true;
	state.selected->initialize();
}

void present()
{
	activeBackend().present();
}

void shutdown()
{
	activeBackend().shutdown();
}

bool hasCapability(const char *capability)
{
	return activeBackend().hasCapability(capability);
}

legacygl::Sink *sink()
{
	return activeBackend().sink();
}

}
