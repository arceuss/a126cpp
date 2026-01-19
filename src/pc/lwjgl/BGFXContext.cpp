#include "BGFXContext.h"

#ifdef USE_BGFX

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#elif defined(__APPLE__)
// macOS uses Cocoa - will need appropriate headers
#endif

namespace lwjgl
{
namespace BGFXContext
{

void* getNativeWindowHandle(SDL_Window* window)
{
	if (!window)
		return nullptr;
	
	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	if (!props)
		return nullptr;
	
#ifdef _WIN32
	// SDL3 on Windows: get HWND pointer
	HWND* hwnd_ptr = (HWND*)SDL_GetProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	if (hwnd_ptr && *hwnd_ptr) {
		return (void*)(*hwnd_ptr);
	}
	return nullptr;
#elif defined(__linux__)
	// Linux - X11 or Wayland
	// Try Wayland first
	void* surface = SDL_GetProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
	if (surface) return surface;
	
	// Try X11
	uint64_t xid = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
	if (xid) return (void*)(uintptr_t)xid;
	return nullptr;
#elif defined(__APPLE__)
	// macOS Cocoa window
	return SDL_GetProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#else
	return nullptr;
#endif
}

bool init(SDL_Window* window, uint32_t width, uint32_t height)
{
	if (!window)
		return false;
	
	// Get native window handle
	void* nativeWindowHandle = getNativeWindowHandle(window);
	if (!nativeWindowHandle)
	{
		// Fallback: try to get window ID
		SDL_PropertiesID props = SDL_GetWindowProperties(window);
		nativeWindowHandle = (void*)(uintptr_t)SDL_GetWindowID(window);
	}
	
	if (!nativeWindowHandle)
		return false;
	
	// Initialize bgfx
	bgfx::Init init;
	
	// Set renderer type - prefer Vulkan, fallback to OpenGL
	init.type = bgfx::RendererType::Vulkan;
	
	// Platform data for SDL window
	bgfx::PlatformData pd;
	pd.nwh = nativeWindowHandle;
	
#ifdef _WIN32
	// Windows: get HDC from window handle
	HWND hwnd = (HWND)nativeWindowHandle;
	pd.ndt = GetDC(hwnd);
	// Also get HDC from SDL properties for more reliable access
	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	if (props) {
		HDC* hdc_ptr = (HDC*)SDL_GetProperty(props, SDL_PROP_WINDOW_WIN32_HDC_POINTER, nullptr);
		if (hdc_ptr && *hdc_ptr) {
			pd.ndt = *hdc_ptr;
		}
	}
#elif defined(__linux__)
	// Linux: X11 Display - try to get from properties
	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	if (props) {
		pd.ndt = SDL_GetProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
	}
	if (!pd.ndt) {
		pd.ndt = nullptr; // XOpenDisplay(nullptr) would go here if needed
	}
#elif defined(__APPLE__)
	pd.ndt = nullptr;
#endif
	
	pd.context = nullptr;
	pd.backBuffer = nullptr;
	pd.backBufferDS = nullptr;
	
	init.platformData = pd;
	
	// Set resolution
	init.resolution.width = width;
	init.resolution.height = height;
	init.resolution.reset = BGFX_RESET_VSYNC;
	
	// Initialize bgfx
	if (!bgfx::init(init))
	{
		// Try OpenGL fallback if Vulkan fails
		init.type = bgfx::RendererType::OpenGL;
		if (!bgfx::init(init))
		{
			return false;
		}
	}
	
	// Enable debug text
	bgfx::setDebug(BGFX_DEBUG_TEXT);
	
	return true;
}

void resize(uint32_t width, uint32_t height)
{
	bgfx::reset(width, height, BGFX_RESET_VSYNC);
}

void shutdown()
{
	bgfx::shutdown();
}

void frame()
{
	bgfx::frame();
}

} // namespace BGFXContext
} // namespace lwjgl

#endif // USE_BGFX
