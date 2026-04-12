// SDL2 platform backend - GL context creation
// Moved from src/pc/lwjgl/GLContext.cpp

#include "lwjgl/GLContext.h"
#include "Backends/Shared/SDL2.h"

#include <iostream>
#include <stdexcept>
#include <csignal>
#include <string>
#include <set>

#include <glad/glad.h>

#include "external/SDLException.h"

#include "SDL.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define IDI_ICON1 1
#endif

// #define MC_DEBUG_GL

#ifdef MC_DEBUG_GL
static void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *msg, const void *data)
{
    const char* _source;
    const char* _type;
    const char* _severity;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
        _source = "API";
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        _source = "WINDOW SYSTEM";
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        _source = "SHADER COMPILER";
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
        _source = "THIRD PARTY";
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
        _source = "APPLICATION";
        break;

        case GL_DEBUG_SOURCE_OTHER:
        _source = "UNKNOWN";
        break;

        default:
        _source = "UNKNOWN";
        break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
        _type = "ERROR";
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        _type = "DEPRECATED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        _type = "UDEFINED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
        _type = "PORTABILITY";
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
        _type = "PERFORMANCE";
        break;

        case GL_DEBUG_TYPE_OTHER:
        _type = "OTHER";
        break;

        case GL_DEBUG_TYPE_MARKER:
        _type = "MARKER";
        break;

        default:
        _type = "UNKNOWN";
        break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        _severity = "HIGH";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        _severity = "MEDIUM";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        _severity = "LOW";
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        _severity = "NOTIFICATION";
        break;

        default:
        _severity = "UNKNOWN";
        break;
    }

    printf("%d: %s of %s severity, raised from %s: %s\n",
            id, _type, _severity, _source, msg);
	std::raise(SIGINT);
}
#endif

// Shared SDL2 state storage
namespace SDL2_Shared
{
    static SDL_Window* s_window = nullptr;
    static SDL_GLContext s_gl_context = nullptr;

    SDL_Window* getWindow() { return s_window; }
    SDL_GLContext getGLContext() { return s_gl_context; }
    void setWindow(SDL_Window* window) { s_window = window; }
    void setGLContext(SDL_GLContext context) { s_gl_context = context; }
}

namespace lwjgl
{
namespace GLContext
{

// Detail implementation
namespace detail
{

static void setContextAttributes(int major, int minor)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
}

// Context singleton
class GLContext
{
private:
	SDL_Window *window = nullptr;
	SDL_GLContext gl_context = nullptr;
	GLCapabilities capabilties;

public:
	GLContext()
	{
		// Prefer a 2.1 compatibility context and fall back to the legacy path if needed.
		setContextAttributes(2, 1);

		// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

#ifdef MC_DEBUG_GL
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

		// Create SDL window
		window = SDL_CreateWindow("Minecraft Alpha v1.2.6", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 854, 480, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		if (window == nullptr)
			throw SDLException();

		// Load and set window icon
#ifdef _WIN32
		{
			HICON hIcon = NULL;
			bool fromResource = false;

			// First try to load from resource (embedded icon)
			hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
			if (hIcon != NULL)
			{
				fromResource = true;
			}
			else
			{
				// If not found in resource, try loading from file
				const char* iconPaths[] = {
					"src/mc.ico",
					"mc.ico",
					"../src/mc.ico",
					"../../src/mc.ico"
				};

				for (int i = 0; i < 4 && hIcon == NULL; i++)
				{
					hIcon = (HICON)LoadImageA(NULL, iconPaths[i], IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
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
						int width = bmp.bmWidth;
						int height = bmp.bmHeight;

						// Create SDL surface from icon (BGRA32 format for Windows)
						SDL_Surface *iconSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_BGRA32);
						if (iconSurface != nullptr)
						{
							// Get bitmap bits
							HDC hDC = CreateCompatibleDC(NULL);
							HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, iconInfo.hbmColor);

							BITMAPINFO bmi;
							ZeroMemory(&bmi, sizeof(BITMAPINFO));
							bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
							bmi.bmiHeader.biWidth = width;
							bmi.bmiHeader.biHeight = -height; // Negative for top-down DIB
							bmi.bmiHeader.biPlanes = 1;
							bmi.bmiHeader.biBitCount = 32;
							bmi.bmiHeader.biCompression = BI_RGB;

							if (GetDIBits(hDC, iconInfo.hbmColor, 0, height, iconSurface->pixels, &bmi, DIB_RGB_COLORS))
							{
								SDL_SetWindowIcon(window, iconSurface);
							}

							SelectObject(hDC, hOldBmp);
							DeleteDC(hDC);
							SDL_FreeSurface(iconSurface);
						}
					}

					if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
					if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
				}

				// Only destroy if we loaded from file (not from resource)
				if (!fromResource)
				{
					DestroyIcon(hIcon);
				}
			}
		}
#endif

		// Create OpenGL context
		gl_context = SDL_GL_CreateContext(window);
		if (gl_context == nullptr)
		{
			std::string requestedContextError = SDL_GetError();
			SDL_ClearError();

			setContextAttributes(1, 1);
			gl_context = SDL_GL_CreateContext(window);
			if (gl_context == nullptr)
				throw SDLException();

			if (!requestedContextError.empty())
				std::cerr << "OpenGL 2.1 compatibility context unavailable, falling back to 1.1: " << requestedContextError << '\n';
		}

		if (SDL_GL_MakeCurrent(window, gl_context))
			throw SDLException();

		// Load GLAD
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
			throw std::runtime_error("Failed to load glad");

		// Disable VSync
		SDL_GL_SetSwapInterval(0);

		// Parse capabilities
		{
			const GLubyte *extensions = glad_glGetString(GL_EXTENSIONS);
			if (extensions != nullptr)
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

		// Store in shared state
		SDL2_Shared::setWindow(window);
		SDL2_Shared::setGLContext(gl_context);

#ifdef MC_DEBUG_GL
		// Enable debugging
		glad_glEnable(GL_DEBUG_OUTPUT);
		glad_glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glad_glDebugMessageCallback(GLDebugMessageCallback, nullptr);
#endif
	}

	SDL_Window *getWindow() const { return window; }
	SDL_GLContext getGLContext() const { return gl_context; }
	const GLCapabilities &getCapabilities() const { return capabilties; }
};

// Context singletons
static GLContext &getContext()
{
	static GLContext context;
	return context;
}

SDL_Window *getWindow()
{
	return getContext().getWindow();
}
SDL_GLContext getGLContext()
{
	return getContext().getGLContext();
}

}

// GL capabilities
void instantiate()
{
	detail::getContext();
	if (SDL_GL_MakeCurrent(detail::getContext().getWindow(), detail::getContext().getGLContext()))
		throw SDLException();
}

const detail::GLCapabilities &getCapabilities()
{
	return detail::getContext().getCapabilities();
}

}
}
