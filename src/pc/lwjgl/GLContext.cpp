#include "lwjgl/GLContext.h"

#include <iostream>
#include <stdexcept>

#include "external/SDLException.h"

// SDL 1.2 OpenGL context is tied to the current video surface.

namespace lwjgl
{
namespace GLContext
{

// Detail implementation
namespace detail
{

class GLContext
{
private:
    SDL_Surface *screen = nullptr;
    GLCapabilities capabilties;

public:
    GLContext()
    {
        // Basic OpenGL attributes for SDL 1.2.
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        // Try to disable vsync where supported.
#ifdef SDL_GL_SWAP_CONTROL
        // SDL 1.2 uses SDL_GL_SWAP_CONTROL attribute when built with that support.
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
#endif

        if (!setVideoMode(854, 480, false, true))
            throw SDLException();

        // Load GLAD (must happen after the OpenGL context exists).
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
            throw std::runtime_error("Failed to load glad");

        // Parse capabilities
        const GLubyte *extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            std::string cap;
            const char *extension_p = reinterpret_cast<const char *>(extensions);
            while (*extension_p != '\0')
            {
                if (*extension_p == ' ')
                {
                    if (!cap.empty())
                    {
                        capabilties.add(cap);
                        cap.clear();
                    }
                    while (*extension_p == ' ')
                        extension_p++;
                    continue;
                }
                cap.push_back(*extension_p++);
            }
            if (!cap.empty())
                capabilties.add(cap);
        }
    }

    SDL_Surface *getScreen() const { return screen; }
    const GLCapabilities &getCapabilities() const { return capabilties; }

    bool setVideoMode(int width, int height, bool fullscreen, bool resizable)
    {
        Uint32 flags = SDL_OPENGL;
        if (fullscreen)
            flags |= SDL_FULLSCREEN;
        if (resizable && !fullscreen)
            flags |= SDL_RESIZABLE;

        // bpp = 0 lets SDL choose the current desktop format.
        SDL_Surface *new_screen = SDL_SetVideoMode(width, height, 0, flags);
        if (!new_screen)
            return false;

        screen = new_screen;

        SDL_WM_SetCaption("Minecraft Alpha v1.2.6", nullptr);
        return true;
    }
};

static GLContext &getContext()
{
    static GLContext context;
    return context;
}

SDL_Surface *getScreen()
{
    return getContext().getScreen();
}

bool setVideoMode(int width, int height, bool fullscreen, bool resizable)
{
    return getContext().setVideoMode(width, height, fullscreen, resizable);
}

} // namespace detail

void instantiate()
{
    // Force construction.
    (void)detail::getScreen();
}

const detail::GLCapabilities &getCapabilities()
{
    return detail::getContext().getCapabilities();
}

} // namespace GLContext
} // namespace lwjgl
