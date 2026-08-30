#pragma once

namespace platform
{

enum class WindowGraphicsAPI
{
	OpenGL,
	Vulkan
};

void initialize();
void shutdown();

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
// platform implementations only forward the native instance and write the
// surface handle into caller-owned VkSurfaceKHR storage.
void getRequiredVulkanInstanceExtensions(unsigned int &count, const char **names);
void createVulkanSurface(void *instance, void *surfaceStorage);

void pumpEvents();
bool isCloseRequested();

}
