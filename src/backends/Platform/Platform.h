#pragma once

#include <string>

namespace platform
{

enum class WindowGraphicsAPI
{
	OpenGL,
	Vulkan,
	Direct3D
};

void initialize();
void shutdown();
std::string getCachePath(const char *fileName);

// The selected renderer chooses the graphics API, while the platform owns the
// native window and all window-system operations.
void createWindow(WindowGraphicsAPI graphicsAPI);
void destroyWindow();

void showWindow();
void hideWindow();
void setWindowSize(int width, int height);
void setFullscreen(bool fullscreen);
bool isWindowVisible();
bool isWindowFocused();
void getWindowPosition(int &x, int &y);
void getWindowSize(int &width, int &height);
void getFullscreenDisplayMode(int &width, int &height, int &bitsPerPixel, int &frequency);
void getDrawableSize(int &width, int &height);
void setCursorPosition(int x, int y);

// Opaque Vulkan window-system bridge. Vulkan types stay in the renderer;
// platform implementations expose the loader entry point, forward the native
// instance and write the surface handle into caller-owned VkSurfaceKHR storage.
void *getVulkanInstanceProcAddress();
void getRequiredVulkanInstanceExtensions(unsigned int &count, const char **names);
void createVulkanSurface(void *instance, void *surfaceStorage);

#ifdef _WIN32
// Opaque Win32 window-system bridge. Win32 types stay in the renderer and
// platform implementation instead of leaking into the shared interface.
void *getWin32WindowHandle();
#endif

void pumpEvents();
bool isCloseRequested();

}
