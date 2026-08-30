#pragma once

#include "legacygl/Context.h"
#include "legacygl/LegacyGL.h"

// Shared setup for the LegacyGL semantic tests.
//
// These tests exercise the real frontend entry points, not a parallel test
// implementation, so they cover the same code the game calls. The null backend
// keeps them GPU-free: state, errors, display lists and queries all behave
// exactly as they do with a real backend, and it hands out deterministic object
// names so expected values can be written down.

namespace legacyglTest
{

inline legacygl::Context &begin()
{
	legacygl::Context &ctx = legacygl::context();
	ctx.reset();
	legacygl::setSink(legacygl::nullSink());
	return ctx;
}

// Compares a matrix against expected column-major contents.
inline bool matrixEquals(const legacygl::Mat4 &matrix, const float (&expected)[16], float tolerance)
{
	for (int i = 0; i < 16; i++)
	{
		const float difference = matrix.m[i] - expected[i];
		if (difference > tolerance || difference < -tolerance)
			return false;
	}
	return true;
}

}
