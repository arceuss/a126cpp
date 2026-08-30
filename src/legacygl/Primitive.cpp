#include "legacygl/Primitive.h"

#include "legacygl/LegacyGL.h"

namespace legacygl
{

static void emitPoint(PrimitiveBatch &out, int a)
{
	CanonicalPrimitive p;
	p.indices[0] = a;
	p.indices[1] = -1;
	p.indices[2] = -1;
	p.provoking = a;
	out.primitives.push_back(p);
}

static void emitLine(PrimitiveBatch &out, int a, int b, int provoking)
{
	CanonicalPrimitive p;
	p.indices[0] = a;
	p.indices[1] = b;
	p.indices[2] = -1;
	p.provoking = provoking;
	out.primitives.push_back(p);
}

static void emitTriangle(PrimitiveBatch &out, int a, int b, int c, int provoking)
{
	CanonicalPrimitive p;
	p.indices[0] = a;
	p.indices[1] = b;
	p.indices[2] = c;
	p.provoking = provoking;
	out.primitives.push_back(p);
}

bool isSupportedPrimitiveMode(unsigned int mode)
{
	switch (mode)
	{
		case GL_POINTS:
		case GL_LINES:
		case GL_LINE_LOOP:
		case GL_LINE_STRIP:
		case GL_TRIANGLES:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLE_FAN:
		case GL_QUADS:
		case GL_QUAD_STRIP:
		case GL_POLYGON:
			return true;
		default:
			return false;
	}
}

bool canonicalizePrimitives(unsigned int mode, int vertexCount, PrimitiveBatch &out)
{
	out.primitives.clear();
	out.sourceMode = mode;

	if (!isSupportedPrimitiveMode(mode))
		return false;

	if (vertexCount < 0)
		vertexCount = 0;

	switch (mode)
	{
		case GL_POINTS:
			out.topology = Topology::Points;
			for (int i = 0; i < vertexCount; i++)
				emitPoint(out, i);
			return true;

		case GL_LINES:
			out.topology = Topology::Lines;
			for (int i = 0; i + 1 < vertexCount; i += 2)
				emitLine(out, i, i + 1, i + 1);
			return true;

		case GL_LINE_STRIP:
			out.topology = Topology::Lines;
			for (int i = 0; i + 1 < vertexCount; i++)
				emitLine(out, i, i + 1, i + 1);
			return true;

		case GL_LINE_LOOP:
			out.topology = Topology::Lines;
			if (vertexCount < 2)
				return true;
			for (int i = 0; i + 1 < vertexCount; i++)
				emitLine(out, i, i + 1, i + 1);
			// The closing segment's provoking vertex is the first submitted
			// vertex, which is why a naive strip conversion recolours it.
			emitLine(out, vertexCount - 1, 0, 0);
			return true;

		case GL_TRIANGLES:
			out.topology = Topology::Triangles;
			for (int i = 0; i + 2 < vertexCount; i += 3)
				emitTriangle(out, i, i + 1, i + 2, i + 2);
			return true;

		case GL_TRIANGLE_STRIP:
			out.topology = Topology::Triangles;
			for (int i = 0; i + 2 < vertexCount; i++)
			{
				// Odd triangles swap their first two vertices so the winding
				// stays consistent, but the provoking vertex is still the
				// newest one.
				if ((i & 1) == 0)
					emitTriangle(out, i, i + 1, i + 2, i + 2);
				else
					emitTriangle(out, i + 1, i, i + 2, i + 2);
			}
			return true;

		case GL_TRIANGLE_FAN:
			out.topology = Topology::Triangles;
			for (int i = 1; i + 1 < vertexCount; i++)
				emitTriangle(out, 0, i, i + 1, i + 1);
			return true;

		case GL_QUADS:
			out.topology = Topology::Triangles;
			for (int i = 0; i + 3 < vertexCount; i += 4)
			{
				// Both halves of the quad carry the quad's fourth vertex as the
				// flat colour source. A conversion that used each triangle's own
				// last vertex would give the first half vertex i+2's colour.
				emitTriangle(out, i, i + 1, i + 2, i + 3);
				emitTriangle(out, i, i + 2, i + 3, i + 3);
			}
			return true;

		case GL_QUAD_STRIP:
			out.topology = Topology::Triangles;
			for (int i = 0; i + 3 < vertexCount; i += 2)
			{
				// Quad k spans vertices 2k, 2k+1, 2k+3, 2k+2 in GL order; the
				// provoking vertex is 2k+3.
				emitTriangle(out, i, i + 1, i + 3, i + 3);
				emitTriangle(out, i, i + 3, i + 2, i + 3);
			}
			return true;

		case GL_POLYGON:
			out.topology = Topology::Triangles;
			for (int i = 1; i + 1 < vertexCount; i++)
				emitTriangle(out, 0, i, i + 1, 0);
			return true;

		default:
			return false;
	}
}

}
