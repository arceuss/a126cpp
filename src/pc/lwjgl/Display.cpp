#include "lwjgl/Display.h"

#include <iostream>
#include <stdexcept>

#include "lwjgl/GLContext.h"
#include "lwjgl/Mouse.h"
#include "lwjgl/Keyboard.h"

#include "external/SDLException.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#ifdef USE_BGFX
#include "lwjgl/BGFXContext.h"
#endif

namespace lwjgl
{
namespace Display
{

static bool close_requested = false;

static DisplayMode current_display_mode(0, 0);

// Display functions
void setDisplayMode(const DisplayMode &display_mode)
{
	if (!display_mode.isFullscreen())
	{
		SDL_SetWindowSize(GLContext::detail::getWindow(), display_mode.getWidth(), display_mode.getHeight());
	}
	current_display_mode = display_mode;
	setFullscreen(display_mode.isFullscreen());
}

DisplayMode getDisplayMode()
{
	return current_display_mode;
}

void setTitle(const jstring &string)
{
	// I guess this gets ignored in favor of the frame title
	// SDL_SetWindowTitle(GLContext::detail::getWindow(), string.c_str());
}

void setFullscreen(bool fullscreen)
{
	if (!SDL_SetWindowFullscreen(GLContext::detail::getWindow(), fullscreen))
		throw SDLException();
	
	// Update display mode
	if (fullscreen)
	{
		int w, h;
		int freq, bpp;

		// In SDL3, get the current display mode from the window's display
		const SDL_DisplayMode *sdl_mode = SDL_GetWindowFullscreenMode(GLContext::detail::getWindow());
		if (sdl_mode == nullptr)
		{
			// If no exclusive fullscreen mode is set, use desktop display mode
			SDL_DisplayID display_id = SDL_GetDisplayForWindow(GLContext::detail::getWindow());
			sdl_mode = SDL_GetDesktopDisplayMode(display_id);
			if (sdl_mode == nullptr)
				throw SDLException();
		}

		w = sdl_mode->w;
		h = sdl_mode->h;
		freq = (int)sdl_mode->refresh_rate;
		bpp = SDL_BITSPERPIXEL(sdl_mode->format);

		current_display_mode = DisplayMode(w, h, bpp, freq);
	}
	else
	{
		int w, h;
		SDL_GetWindowSize(GLContext::detail::getWindow(), &w, &h);
		current_display_mode = DisplayMode(w, h);
	}
}

bool isCloseRequested()
{
	return close_requested;
}

bool isVisible()
{
	auto flags = SDL_GetWindowFlags(GLContext::detail::getWindow());
	return (flags & SDL_WINDOW_HIDDEN) == 0;
}

bool isActive()
{
	auto flags = SDL_GetWindowFlags(GLContext::detail::getWindow());
	return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void processMessages()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_EVENT_QUIT:
				close_requested = true;
				break;
			case SDL_EVENT_MOUSE_MOTION:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_WHEEL:
				Mouse::detail::pushEvent(e);
				break;
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
			case SDL_EVENT_TEXT_INPUT:
				Keyboard::detail::pushEvent(e);
				break;
		}
	}

	// Update display mode
	if (!current_display_mode.isFullscreen())
	{
		int w, h;
		SDL_GetWindowSize(GLContext::detail::getWindow(), &w, &h);
		current_display_mode = DisplayMode(w, h);
	}
}

void swapBuffers()
{
#ifdef USE_BGFX
	// When using bgfx, call bgfx::frame() instead of SDL_GL_SwapWindow
	BGFXContext::frame();
#else
	SDL_GL_SwapWindow(GLContext::detail::getWindow());
#endif
}

void update(bool doProcessMessages)
{
	swapBuffers();
	if (doProcessMessages)
		processMessages();
}

void create()
{
	SDL_ShowWindow(GLContext::detail::getWindow());
}

int_t getX()
{
	int x;
	SDL_GetWindowPosition(GLContext::detail::getWindow(), &x, nullptr);
	return x;
}

int_t getY()
{
	int y;
	SDL_GetWindowPosition(GLContext::detail::getWindow(), nullptr, &y);
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
