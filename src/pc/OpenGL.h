#pragma once

#ifdef USE_BGFX
// When using bgfx, redirect OpenGL calls to compatibility layer
#include "OpenGL_BGFX.h"
#else
// When not using bgfx, use real OpenGL
#include <glad/glad.h>
#endif

// Beta: GL_RESCALE_NORMAL constant (GL_EXT_rescale_normal = 32826)
// Use numeric value to match Beta exactly
// Some OpenGL implementations may not define this constant
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif