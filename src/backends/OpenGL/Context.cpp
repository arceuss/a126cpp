#include "backends/OpenGL/Context.h"

#include <csignal>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

#include <glad/glad.h>

#include "backends/Backend.h"
#include "backends/Platform/Platform.h"
#include "backends/Platform/SDL2/Platform.h"
#include "pc/external/SDLException.h"

#include "SDL.h"

// #define MC_DEBUG_GL

namespace openglbackend
{

static SDL_GLContext glContext = nullptr;
static std::set<std::string> capabilities;

#ifdef MC_DEBUG_GL
static void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar *msg, const void *data)
{
	const char *_source;
	const char *_type;
	const char *_severity;

	switch (source)
	{
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

	switch (type)
	{
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

	switch (severity)
	{
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

void initialize(const renderbackend::Configuration &backend)
{
	if (glContext != nullptr)
	{
		SDL_Window *window = platform::sdl2::window();
		if (SDL_GL_MakeCurrent(window, glContext))
			throw SDLException();
		return;
	}

	std::cout << "legacygl: selected backend " << backend.recordName << '\n';

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, backend.requestedGLMajorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, backend.requestedGLMinorVersion);
	if (backend.requestedGLProfile == renderbackend::OpenGLProfile::Core)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	}

	// Pin the framebuffer format instead of taking whatever the driver
	// defaults to, so every platform renders into a comparable target. These
	// are the values a desktop NVIDIA driver picks on its own, measured as
	// "rgba=8/8/8/0 depth=24 stencil=0". Switch EGL otherwise hands out a
	// 16-bit depth buffer, which z-fights visibly on Alpha's cloud plane.
	//
	// Alpha is not set here: SDL's default request is 0, which is what the
	// game wants since it never reads destination alpha, and the GPU parity
	// fixture requests 8 before this runs because its cases do read it.
	// Switch reports rgba=8/8/8/8 either way: its native window surface is
	// RGBA8888, so EGL has no alpha-less config to offer. Depth and stencil
	// do match desktop.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

#ifdef MC_DEBUG_GL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	platform::createWindow(platform::WindowGraphicsAPI::OpenGL);
	SDL_Window *window = platform::sdl2::window();

	glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr)
		throw SDLException();

	if (SDL_GL_MakeCurrent(window, glContext))
		throw SDLException();

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
		throw std::runtime_error("Failed to load glad");

	GLint profileMask = 0;
	if (GLAD_GL_VERSION_3_2)
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);

	if (backend.requiredGLMajorVersion != 0)
	{
		GLint major = 0;
		GLint minor = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		const bool loaderVersionTooOld =
			GLVersion.major < backend.requiredGLMajorVersion ||
			(GLVersion.major == backend.requiredGLMajorVersion &&
				GLVersion.minor < backend.requiredGLMinorVersion);
		const bool driverVersionTooOld =
			major < backend.requiredGLMajorVersion ||
			(major == backend.requiredGLMajorVersion && minor < backend.requiredGLMinorVersion);
		if (loaderVersionTooOld || driverVersionTooOld)
		{
			throw std::runtime_error(std::string("The ") + backend.recordName +
				" backend requires OpenGL " + std::to_string(backend.requiredGLMajorVersion) + "." +
				std::to_string(backend.requiredGLMinorVersion));
		}
	}

	if (backend.requiredGLProfile == renderbackend::OpenGLProfile::Core &&
		(profileMask & GL_CONTEXT_CORE_PROFILE_BIT) == 0)
	{
		throw std::runtime_error(std::string("The ") + backend.recordName +
			" backend requires a Core-profile context; compatibility fallback refused");
	}
	if (backend.requiredGLProfile == renderbackend::OpenGLProfile::Compatibility &&
		(profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0)
	{
		throw std::runtime_error(std::string("The ") + backend.recordName +
			" backend requires a compatibility-profile context; Core fallback refused");
	}

	SDL_GL_SetSwapInterval(0);

	capabilities.clear();
	if (GLAD_GL_VERSION_3_0 && glad_glGetStringi != nullptr)
	{
		GLint count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &count);
		for (GLint i = 0; i < count; i++)
		{
			const GLubyte *extension = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
			if (extension != nullptr)
				capabilities.insert(reinterpret_cast<const char *>(extension));
		}
	}
	else
	{
		const GLubyte *extensions = glGetString(GL_EXTENSIONS);
		if (extensions != nullptr)
		{
			const char *extension = reinterpret_cast<const char *>(extensions);
			while (*extension != '\0')
			{
				while (*extension == ' ')
					extension++;
				const char *start = extension;
				while (*extension != '\0' && *extension != ' ')
					extension++;
				if (extension != start)
					capabilities.insert(std::string(start, extension));
			}
		}
	}

	// This is a frontend capability on the translated backend: the shader
	// implements radial fog even when the Core driver does not advertise the
	// compatibility extension. Keeping it visible preserves the game call
	// stream selected at GameRenderer::setupFog.
	if (backend.virtualNVFogDistance)
		capabilities.insert("GL_NV_fog_distance");

#ifdef MC_DEBUG_GL
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GLDebugMessageCallback, nullptr);
#endif

	const auto glString = [](GLenum name)
	{
		const GLubyte *value = glGetString(name);
		return value == nullptr ? "<unavailable>" : reinterpret_cast<const char *>(value);
	};
	const char *profile = "legacy";
	if ((profileMask & GL_CONTEXT_CORE_PROFILE_BIT) != 0)
		profile = "core";
	else if ((profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0)
		profile = "compatibility";
	else if (GLAD_GL_VERSION_3_2)
		profile = "unknown";

	std::cout << "legacygl: GL vendor=" << glString(GL_VENDOR)
		<< " renderer=" << glString(GL_RENDERER)
		<< " version=" << glString(GL_VERSION)
		<< " profile=" << profile << '\n';

	const auto attribute = [](SDL_GLattr name)
	{
		int value = -1;
		if (SDL_GL_GetAttribute(name, &value) != 0)
			return -1;
		return value;
	};
	std::cout << "legacygl: SDL framebuffer rgba="
		<< attribute(SDL_GL_RED_SIZE) << '/' << attribute(SDL_GL_GREEN_SIZE) << '/'
		<< attribute(SDL_GL_BLUE_SIZE) << '/' << attribute(SDL_GL_ALPHA_SIZE)
		<< " depth=" << attribute(SDL_GL_DEPTH_SIZE)
		<< " stencil=" << attribute(SDL_GL_STENCIL_SIZE)
		<< " sampleBuffers=" << attribute(SDL_GL_MULTISAMPLEBUFFERS)
		<< " samples=" << attribute(SDL_GL_MULTISAMPLESAMPLES)
		<< " sRGB=" << attribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE) << '\n';
}

void present()
{
	SDL_GL_SwapWindow(platform::sdl2::window());
}

void shutdown()
{
	if (glContext == nullptr)
		return;
	SDL_GL_DeleteContext(glContext);
	glContext = nullptr;
	capabilities.clear();
}

bool hasCapability(const char *capability)
{
	return capabilities.find(capability) != capabilities.end();
}

}
