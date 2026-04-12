#pragma once

// Platform backend interface
// Includes the platform-agnostic interface headers for windowing, input, and context.
// Exactly one set of Platform/*.cpp files is compiled per build (selected by CMake).

#include "lwjgl/Display.h"
#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"
#include "lwjgl/GLContext.h"
