#pragma once

// The renderer's OpenGL header. It resolves to the LegacyGL frontend rather than
// to a GL loader, so every gl* call the game makes passes through the semantic
// core (legacygl/Context) before reaching a backend. The native
// compatibility-OpenGL backend forwards the same calls in the same order and
// remains the behavioural reference.
//
// GL_RESCALE_NORMAL and GL_RESCALE_NORMAL_EXT are both declared by the frontend,
// with the value 32826 the renderer relies on.

#include "legacygl/LegacyGL.h"
