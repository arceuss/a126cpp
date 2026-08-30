#include "lwjgl/Display.h"

#include "backends/Backend.h"
#include "backends/Platform/Platform.h"

namespace lwjgl
{
namespace Display
{

static DisplayMode current_display_mode(0, 0);

void setDisplayMode(const DisplayMode &display_mode)
{
	if (!display_mode.isFullscreen())
		platform::setWindowSize(display_mode.getWidth(), display_mode.getHeight());
	current_display_mode = display_mode;
	setFullscreen(display_mode.isFullscreen());
}

DisplayMode getDisplayMode()
{
	return current_display_mode;
}

void setTitle(const jstring &string)
{
	// I guess this gets ignored in favor of the frame title.
}

void setFullscreen(bool fullscreen)
{
	platform::setFullscreen(fullscreen);

	if (fullscreen)
	{
		int width;
		int height;
		int bitsPerPixel;
		int frequency;
		platform::getFullscreenDisplayMode(width, height, bitsPerPixel, frequency);
		current_display_mode = DisplayMode(width, height, bitsPerPixel, frequency);
	}
	else
	{
		int width;
		int height;
		platform::getWindowSize(width, height);
		current_display_mode = DisplayMode(width, height);
	}
}

bool isCloseRequested()
{
	return platform::isCloseRequested();
}

bool isVisible()
{
	return platform::isWindowVisible();
}

bool isActive()
{
	return platform::isWindowFocused();
}

void processMessages()
{
	platform::pumpEvents();

	if (!current_display_mode.isFullscreen())
	{
		int width;
		int height;
		platform::getWindowSize(width, height);
		current_display_mode = DisplayMode(width, height);
	}
}

void swapBuffers()
{
	renderbackend::present();
}

void update(bool doProcessMessages)
{
	swapBuffers();
	if (doProcessMessages)
		processMessages();
}

void create()
{
	platform::showWindow();
}

int_t getX()
{
	int x;
	int y;
	platform::getWindowPosition(x, y);
	return x;
}

int_t getY()
{
	int x;
	int y;
	platform::getWindowPosition(x, y);
	return y;
}

int_t getWidth()
{
	return current_display_mode.getWidth();
}

int_t getHeight()
{
	return current_display_mode.getHeight();
}

}
}
