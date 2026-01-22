#pragma once

// Central include for SDL 1.2.
// Keep this as the only place that includes SDL headers directly.

#include "SDL.h"

// Some older toolchains (and/or SDL builds) can be picky about stdint vs SDL types.
// If you run into conflicts, put any project-wide SDL defines here.
