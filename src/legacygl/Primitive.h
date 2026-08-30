#pragma once

#include <vector>

// Legacy primitive canonicalization.
//
// Modern APIs have no quads, quad strips or polygons, and D3D12 has no triangle
// fan. Converting them is not just an index rewrite: fixed-function flat shading
// takes the primary colour from a topology-specific provoking vertex, so every
// generated triangle or line has to remember which vertex of the *original*
// primitive owns its flat colour. That mapping is computed once here and shared
// by every backend rather than being reinvented per API.
//
// Provoking vertices follow the OpenGL 1.1 specification's flatshading table
// (section 2.7, table 2.4). Note that GL_POLYGON's provoking vertex is the
// first submitted vertex, not the last; see docs/portable/semantic-notes.md.

namespace legacygl
{

enum class Topology
{
	Points,
	Lines,
	Triangles
};

struct CanonicalPrimitive
{
	// Indices into the submitted vertex sequence. Unused entries are -1.
	int indices[3];
	// Index of the original primitive's provoking vertex, for GL_FLAT.
	int provoking;
};

struct PrimitiveBatch
{
	// Topology the original mode reduces to.
	Topology topology = Topology::Triangles;
	// The GL primitive mode as submitted, kept so a backend can tell an
	// original triangle list from a converted quad list.
	unsigned int sourceMode = 0;
	std::vector<CanonicalPrimitive> primitives;
};

// Returns false for a mode outside the supported legacy set; the caller raises
// GL_INVALID_ENUM. A vertex count that leaves an incomplete trailing primitive
// drops that primitive, exactly as OpenGL does.
bool canonicalizePrimitives(unsigned int mode, int vertexCount, PrimitiveBatch &out);

// True when the mode is one the frontend accepts at all.
bool isSupportedPrimitiveMode(unsigned int mode);

}
