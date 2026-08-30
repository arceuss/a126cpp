#include "backends/Platform/Platform.h"

#include "backends/Platform/SDL2/Platform.h"
#include "pc/external/SDLException.h"
#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"

#include "SDL.h"
#include "SDL_vulkan.h"

#ifdef _WIN32
#include "SDL_syswm.h"
#include <windows.h>
#define IDI_ICON1 1
#endif

namespace platform
{

static SDL_Window *windowHandle = nullptr;
static WindowGraphicsAPI windowGraphicsAPI = WindowGraphicsAPI::OpenGL;
static bool initialized = false;
static bool closeRequested = false;

#ifdef _WIN32
static void setWindowIcon(SDL_Window *window)
{
	HICON hIcon = NULL;
	bool fromResource = false;

	// First try to load from resource (embedded icon).
	hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	if (hIcon != NULL)
	{
		fromResource = true;
	}
	else
	{
		// If not found in resource, try loading from file.
		const char *iconPaths[] = {
			"src/mc.ico",
			"mc.ico",
			"../src/mc.ico",
			"../../src/mc.ico"
		};

		for (int i = 0; i < 4 && hIcon == NULL; i++)
		{
			hIcon = static_cast<HICON>(LoadImageA(NULL, iconPaths[i], IMAGE_ICON, 0, 0,
				LR_LOADFROMFILE | LR_DEFAULTSIZE));
		}
	}

	if (hIcon != NULL)
	{
		ICONINFO iconInfo;
		if (GetIconInfo(hIcon, &iconInfo))
		{
			BITMAP bmp;
			if (GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp))
			{
				const int width = bmp.bmWidth;
				const int height = bmp.bmHeight;
				SDL_Surface *iconSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
					SDL_PIXELFORMAT_BGRA32);
				if (iconSurface != nullptr)
				{
					HDC hDC = CreateCompatibleDC(NULL);
					HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hDC, iconInfo.hbmColor));

					BITMAPINFO bmi;
					ZeroMemory(&bmi, sizeof(BITMAPINFO));
					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth = width;
					bmi.bmiHeader.biHeight = -height;
					bmi.bmiHeader.biPlanes = 1;
					bmi.bmiHeader.biBitCount = 32;
					bmi.bmiHeader.biCompression = BI_RGB;

					if (GetDIBits(hDC, iconInfo.hbmColor, 0, height, iconSurface->pixels, &bmi,
						DIB_RGB_COLORS))
					{
						SDL_SetWindowIcon(window, iconSurface);
					}

					SelectObject(hDC, hOldBmp);
					DeleteDC(hDC);
					SDL_FreeSurface(iconSurface);
				}
			}

			if (iconInfo.hbmColor)
				DeleteObject(iconInfo.hbmColor);
			if (iconInfo.hbmMask)
				DeleteObject(iconInfo.hbmMask);
		}

		if (!fromResource)
			DestroyIcon(hIcon);
	}
}
#endif

void initialize()
{
	if (initialized)
		return;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
		throw SDLException();
	initialized = true;
	closeRequested = false;
}

void shutdown()
{
	if (!initialized)
		return;
	SDL_Quit();
	initialized = false;
}

void createWindow(WindowGraphicsAPI graphicsAPI)
{
	if (windowHandle != nullptr)
		return;

	windowGraphicsAPI = graphicsAPI;
	Uint32 graphicsFlag = 0;
	switch (graphicsAPI)
	{
		case WindowGraphicsAPI::OpenGL:
			graphicsFlag = SDL_WINDOW_OPENGL;
			break;
		case WindowGraphicsAPI::Vulkan:
			graphicsFlag = SDL_WINDOW_VULKAN;
			break;
		case WindowGraphicsAPI::Direct3D:
			break;
	}
	windowHandle = SDL_CreateWindow("Minecraft Alpha v1.2.6", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 854, 480,
		SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | graphicsFlag);
	if (windowHandle == nullptr)
		throw SDLException();

#ifdef _WIN32
	setWindowIcon(windowHandle);
#endif
}

void destroyWindow()
{
	if (windowHandle == nullptr)
		return;
	SDL_DestroyWindow(windowHandle);
	windowHandle = nullptr;
}

void showWindow()
{
	SDL_ShowWindow(windowHandle);
}

void hideWindow()
{
	SDL_HideWindow(windowHandle);
}

void setWindowSize(int width, int height)
{
	SDL_SetWindowSize(windowHandle, width, height);
}

void setFullscreen(bool fullscreen)
{
	if (SDL_SetWindowFullscreen(windowHandle, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))
		throw SDLException();
}

bool isWindowVisible()
{
	return (SDL_GetWindowFlags(windowHandle) & SDL_WINDOW_SHOWN) != 0;
}

bool isWindowFocused()
{
	return (SDL_GetWindowFlags(windowHandle) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void getWindowPosition(int &x, int &y)
{
	SDL_GetWindowPosition(windowHandle, &x, &y);
}

void getWindowSize(int &width, int &height)
{
	SDL_GetWindowSize(windowHandle, &width, &height);
}

void getFullscreenDisplayMode(int &width, int &height, int &bitsPerPixel, int &frequency)
{
	SDL_DisplayMode mode;
	if (SDL_GetWindowDisplayMode(windowHandle, &mode))
		throw SDLException();
	width = mode.w;
	height = mode.h;
	bitsPerPixel = SDL_BITSPERPIXEL(mode.format);
	frequency = mode.refresh_rate;
}

void getDrawableSize(int &width, int &height)
{
	if (windowGraphicsAPI == WindowGraphicsAPI::OpenGL)
		SDL_GL_GetDrawableSize(windowHandle, &width, &height);
	else if (windowGraphicsAPI == WindowGraphicsAPI::Vulkan)
		SDL_Vulkan_GetDrawableSize(windowHandle, &width, &height);
	else
		SDL_GetWindowSize(windowHandle, &width, &height);
}

void *getVulkanInstanceProcAddress()
{
	void *address = SDL_Vulkan_GetVkGetInstanceProcAddr();
	if (address == nullptr)
		throw SDLException();
	return address;
}

void getRequiredVulkanInstanceExtensions(unsigned int &count, const char **names)
{
	if (!SDL_Vulkan_GetInstanceExtensions(windowHandle, &count, names))
		throw SDLException();
}

void createVulkanSurface(void *instance, void *surfaceStorage)
{
	if (!SDL_Vulkan_CreateSurface(windowHandle, reinterpret_cast<VkInstance>(instance),
		static_cast<VkSurfaceKHR *>(surfaceStorage)))
	{
		throw SDLException();
	}
}

#ifdef _WIN32
void *getWin32WindowHandle()
{
	SDL_SysWMinfo info = {};
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(windowHandle, &info) || info.subsystem != SDL_SYSWM_WINDOWS)
		throw SDLException();
	return info.info.win.window;
}
#endif

void setCursorPosition(int x, int y)
{
	SDL_WarpMouseInWindow(windowHandle, x, y);
}

void pumpEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				closeRequested = true;
				break;
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEWHEEL:
				lwjgl::Mouse::detail::pushEvent(event);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_TEXTINPUT:
				lwjgl::Keyboard::detail::pushEvent(event);
				break;
		}
	}
}

bool isCloseRequested()
{
	return closeRequested;
}

namespace sdl2
{

SDL_Window *window()
{
	return windowHandle;
}

}
}
