#pragma once

#include <vector>

// Canonical vertex form and legacy attribute conversion.
//
// The semantic core resolves every submitted vertex - immediate mode or client
// array, client memory or buffer object - into this one representation so that
// display-list capture, the trace and any GPU backend all agree on what was
// drawn. Conversion happens once, here, using the OpenGL 1.1 rules.

namespace legacygl
{

struct Vertex
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float s = 0.0f, t = 0.0f;
	float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
	float nx = 0.0f, ny = 0.0f, nz = 1.0f;
};

// A draw's worth of vertices in submission order, plus which attributes the
// draw actually supplied. An attribute that was not supplied is filled from the
// current attribute state when the geometry is executed, not when it is
// captured: that is what lets a glyph display list be recoloured by a
// surrounding glColor4f.
//
// vertexCount is always the number of vertices the draw submitted. The vertices
// vector is filled only when the active backend consumes resolved geometry; the
// native compatibility backend walks the application's arrays itself, so
// duplicating a chunk's vertices in the semantic core would double
// display-list memory for no observable gain.
struct Geometry
{
	unsigned int mode = 0;
	int vertexCount = 0;
	bool hasColor = false;
	bool hasTexCoord = false;
	bool hasNormal = false;
	std::vector<Vertex> vertices;

	void clear()
	{
		mode = 0;
		vertexCount = 0;
		hasColor = false;
		hasTexCoord = false;
		hasNormal = false;
		vertices.clear();
	}
};

// OpenGL 1.1 table 2.6. Signed formats use (2c+1)/(2^b-1), which is not the
// c/(2^(b-1)-1) rule later versions adopted; the byte normals Tesselator packs
// are read back through this one.
inline float normalizeUnsignedByte(unsigned char c)
{
	return static_cast<float>(c) / 255.0f;
}

inline float normalizeSignedByte(signed char c)
{
	return static_cast<float>(2 * static_cast<int>(c) + 1) / 255.0f;
}

}
