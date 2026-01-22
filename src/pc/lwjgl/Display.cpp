#include "lwjgl/Display.h"

#include <stdexcept>

#include "lwjgl/GLContext.h"
#include "lwjgl/Mouse.h"
#include "lwjgl/Keyboard.h"

#include "external/SDLException.h"
#include "external/SDLCompat.h"

namespace lwjgl
{
namespace Display
{

static bool close_requested = false;
static DisplayMode current_display_mode(854, 480);

void setDisplayMode(const DisplayMode &display_mode)
{
    current_display_mode = display_mode;
    setFullscreen(display_mode.isFullscreen());

    if (!display_mode.isFullscreen())
    {
        if (!GLContext::detail::setVideoMode(display_mode.getWidth(), display_mode.getHeight(), false, true))
            throw SDLException();
    }
}

DisplayMode getDisplayMode()
{
    return current_display_mode;
}

void setTitle(const jstring &string)
{
    // SDL 1.2 uses SDL_WM_SetCaption.
    std::string utf8 = String::toUTF8(string);
    SDL_WM_SetCaption(utf8.c_str(), nullptr);
}

void setFullscreen(bool fullscreen)
{
    if (fullscreen)
    {
        const SDL_VideoInfo *info = SDL_GetVideoInfo();
        int w = info ? info->current_w : current_display_mode.getWidth();
        int h = info ? info->current_h : current_display_mode.getHeight();

        if (!GLContext::detail::setVideoMode(w, h, true, false))
            throw SDLException();

        current_display_mode = DisplayMode(w, h);
    }
    else
    {
        if (!GLContext::detail::setVideoMode(current_display_mode.getWidth(), current_display_mode.getHeight(), false, true))
            throw SDLException();
    }
}

bool isCloseRequested()
{
    return close_requested;
}

bool isVisible()
{
    // SDL 1.2 doesn't track window hidden/shown state in a portable way.
    return true;
}

bool isActive()
{
    Uint8 state = SDL_GetAppState();
    return (state & SDL_APPINPUTFOCUS) != 0;
}

void processMessages()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                close_requested = true;
                break;

            case SDL_VIDEORESIZE:
                // Recreate the OpenGL context at the new size.
                if (!current_display_mode.isFullscreen())
                {
                    if (!GLContext::detail::setVideoMode(e.resize.w, e.resize.h, false, true))
                        throw SDLException();
                    current_display_mode = DisplayMode(e.resize.w, e.resize.h);
                }
                break;

            case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                Mouse::detail::pushEvent(e);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                Keyboard::detail::pushEvent(e);
                break;
        }
    }
}

void swapBuffers()
{
    SDL_GL_SwapBuffers();
}

void update(bool doProcessMessages)
{
    swapBuffers();
    if (doProcessMessages)
        processMessages();
}

void create()
{
    // SDL 1.2 creates the window when the video mode is set.
}

int_t getX()
{
    return 0;
}

int_t getY()
{
    return 0;
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
