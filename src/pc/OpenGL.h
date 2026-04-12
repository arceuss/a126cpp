#pragma once

#include <glad/glad.h>

#ifdef _WIN32
#undef APIENTRY
#endif

// Beta: GL_RESCALE_NORMAL constant (GL_EXT_rescale_normal = 32826)
// Use numeric value to match Beta exactly
// Some OpenGL implementations may not define this constant
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif