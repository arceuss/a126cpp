#pragma once

#include "java/Type.h"

// Per-entity memory for OcclusionCuller. Lives on the entity so a pass can
// answer from the last test without a lookup.
struct OcclusionCullState
{
	// Pass that produced `occluded`; -1 before the first test.
	int_t pass = -1;
	// Level tick through which an entity found visible is drawn untested.
	int_t visibleUntilTick = 0;
	bool occluded = false;
};
