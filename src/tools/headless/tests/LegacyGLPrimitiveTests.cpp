// LegacyGL primitive conversion and provoking-vertex tests.
//
// Modern APIs have no quads, quad strips or polygons, and D3D12 has no triangle
// fan, so every legacy mode has to be decomposed. Flat shading makes the
// decomposition observable: the primary colour of a generated triangle comes
// from the provoking vertex of the *original* primitive, which is not the same
// as the last vertex of the generated one. These tests pin the conversion table
// so all three backends inherit identical geometry.
//
// The provoking vertices follow the OpenGL 1.1 flatshading table (section 2.7).

#include <vector>

#include "legacygl/Primitive.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/tests/LegacyGLFixture.h"

static std::string describe(const legacygl::PrimitiveBatch &batch)
{
	std::string text;
	for (const legacygl::CanonicalPrimitive &primitive : batch.primitives)
	{
		text += '(';
		for (int i = 0; i < 3; i++)
		{
			if (primitive.indices[i] < 0)
				continue;
			if (i != 0)
				text += ',';
			text += std::to_string(primitive.indices[i]);
		}
		text += "->";
		text += std::to_string(primitive.provoking);
		text += ')';
	}
	return text;
}

HEADLESS_TEST(legacygl_primitives, triangle_list_keeps_its_last_vertex_as_provoking)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_TRIANGLES, 7, batch), "triangles are supported");
	ctx.checkEqual(describe(batch), "(0,1,2->2)(3,4,5->5)", "two whole triangles, trailing vertex dropped");
	ctx.check(batch.topology == legacygl::Topology::Triangles, "topology is triangles");
}

HEADLESS_TEST(legacygl_primitives, quads_carry_the_fourth_vertex_into_both_halves)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_QUADS, 8, batch), "quads are supported");
	// The naive conversion would give (0,1,2->2)(0,2,3->3): the first triangle
	// would take vertex 2's colour instead of the quad's vertex 3.
	ctx.checkEqual(describe(batch), "(0,1,2->3)(0,2,3->3)(4,5,6->7)(4,6,7->7)",
		"both halves of each quad provoke from the quad's fourth vertex");
}

HEADLESS_TEST(legacygl_primitives, quad_strip_provokes_from_the_newest_vertex)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_QUAD_STRIP, 6, batch), "quad strips are supported");
	ctx.checkEqual(describe(batch), "(0,1,3->3)(0,3,2->3)(2,3,5->5)(2,5,4->5)",
		"quad strip winding and provoking vertices");
}

HEADLESS_TEST(legacygl_primitives, triangle_strip_alternates_winding)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_TRIANGLE_STRIP, 5, batch), "triangle strips are supported");
	// Odd triangles swap their first two vertices so the facing stays
	// consistent; the provoking vertex is still the newest one.
	ctx.checkEqual(describe(batch), "(0,1,2->2)(2,1,3->3)(2,3,4->4)", "strip winding and provoking vertices");
}

HEADLESS_TEST(legacygl_primitives, triangle_fan_shares_the_first_vertex)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_TRIANGLE_FAN, 5, batch), "fans are supported");
	ctx.checkEqual(describe(batch), "(0,1,2->2)(0,2,3->3)(0,3,4->4)", "fan conversion for backends without fans");
}

HEADLESS_TEST(legacygl_primitives, line_loop_closes_with_the_first_vertex)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_LINE_LOOP, 4, batch), "line loops are supported");
	// The closing segment's provoking vertex is the first submitted vertex, so
	// treating a loop as a strip plus one extra segment recolours it.
	ctx.checkEqual(describe(batch), "(0,1->1)(1,2->2)(2,3->3)(3,0->0)", "loop closure and provoking vertices");
	ctx.check(batch.topology == legacygl::Topology::Lines, "topology is lines");

	legacygl::canonicalizePrimitives(GL_LINE_LOOP, 1, batch);
	ctx.checkEqual(static_cast<long long>(batch.primitives.size()), 0, "a single vertex makes no loop");
}

HEADLESS_TEST(legacygl_primitives, lines_and_strips_provoke_from_their_end_vertex)
{
	legacygl::PrimitiveBatch batch;
	legacygl::canonicalizePrimitives(GL_LINES, 5, batch);
	ctx.checkEqual(describe(batch), "(0,1->1)(2,3->3)", "independent lines drop the odd vertex");

	legacygl::canonicalizePrimitives(GL_LINE_STRIP, 4, batch);
	ctx.checkEqual(describe(batch), "(0,1->1)(1,2->2)(2,3->3)", "line strip segments");
}

HEADLESS_TEST(legacygl_primitives, polygon_provokes_from_its_first_vertex)
{
	legacygl::PrimitiveBatch batch;
	ctx.check(legacygl::canonicalizePrimitives(GL_POLYGON, 5, batch), "polygons are supported");
	// OpenGL 1.1's flatshading table gives a single polygon vertex 1 as the
	// provoking vertex, unlike every other topology.
	ctx.checkEqual(describe(batch), "(0,1,2->0)(0,2,3->0)(0,3,4->0)", "polygon fan with a first-vertex colour");
}

HEADLESS_TEST(legacygl_primitives, points_and_degenerate_counts_are_handled)
{
	legacygl::PrimitiveBatch batch;
	legacygl::canonicalizePrimitives(GL_POINTS, 3, batch);
	ctx.checkEqual(describe(batch), "(0->0)(1->1)(2->2)", "points provoke from themselves");
	ctx.check(batch.topology == legacygl::Topology::Points, "topology is points");

	legacygl::canonicalizePrimitives(GL_TRIANGLES, 2, batch);
	ctx.checkEqual(static_cast<long long>(batch.primitives.size()), 0, "an incomplete triangle is dropped");
	legacygl::canonicalizePrimitives(GL_QUADS, 3, batch);
	ctx.checkEqual(static_cast<long long>(batch.primitives.size()), 0, "an incomplete quad is dropped");
	legacygl::canonicalizePrimitives(GL_TRIANGLES, -5, batch);
	ctx.checkEqual(static_cast<long long>(batch.primitives.size()), 0, "a negative count produces nothing");

	ctx.check(!legacygl::canonicalizePrimitives(0x4321, 3, batch), "an unknown mode is refused");
}

HEADLESS_TEST(legacygl_primitives, array_draw_decodes_the_tesselator_layout)
{
	legacygl::Context &gl = legacyglTest::begin();

	// Tesselator's interleaved layout: position float3 at 0, texture float2 at
	// 12, colour ubyte4 at 20, packed byte normal at 24, stride 32.
	struct TesselatorVertex
	{
		float x, y, z;
		float u, v;
		unsigned char rgba[4];
		signed char normal[4];
		// Tesselator advances eight floats per vertex and leaves the last one
		// unused, so the fixture has to carry the same tail.
		unsigned char unused[4];
	};
	static_assert(sizeof(TesselatorVertex) == 32, "the fixture must match Tesselator's stride");

	TesselatorVertex vertices[3];
	for (int i = 0; i < 3; i++)
	{
		vertices[i].x = static_cast<float>(i);
		vertices[i].y = 0.0f;
		vertices[i].z = 0.0f;
		vertices[i].u = 0.25f * i;
		vertices[i].v = 0.5f;
		vertices[i].rgba[0] = 255;
		vertices[i].rgba[1] = 128;
		vertices[i].rgba[2] = 0;
		vertices[i].rgba[3] = 255;
		vertices[i].normal[0] = 127;
		vertices[i].normal[1] = 0;
		vertices[i].normal[2] = -128;
		vertices[i].normal[3] = 0;
	}

	const char *base = reinterpret_cast<const char *>(vertices);
	glVertexPointer(3, GL_FLOAT, 32, base);
	glTexCoordPointer(2, GL_FLOAT, 32, base + 12);
	glColorPointer(4, GL_UNSIGNED_BYTE, 32, base + 20);
	glNormalPointer(GL_BYTE, 32, base + 24);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	const legacygl::Geometry &drawn = gl.lastGeometry();
	ctx.checkEqual(static_cast<long long>(drawn.vertices.size()), 3, "three vertices decoded");
	if (drawn.vertices.size() == 3)
	{
		ctx.checkEqualBits(drawn.vertices[2].x, 2.0f, "positions are read at the right stride");
		ctx.checkEqualBits(drawn.vertices[1].s, 0.25f, "texture coordinates are unnormalized");
		// Colours are normalized unsigned bytes: c/255.
		ctx.checkEqualBits(drawn.vertices[0].r, 1.0f, "255 becomes 1.0");
		ctx.checkEqualBits(drawn.vertices[0].g, 128.0f / 255.0f, "128 becomes 128/255");
		// Normals are normalized signed bytes: (2c+1)/255.
		ctx.checkEqualBits(drawn.vertices[0].nx, 1.0f, "the packed normal x decodes to one");
		ctx.checkEqualBits(drawn.vertices[0].nz, -1.0f, "the packed normal z decodes to minus one");
	}
	ctx.check(drawn.hasColor && drawn.hasNormal && drawn.hasTexCoord, "all four arrays were reported as supplied");

	// Stride zero means tightly packed, which is a different address pattern.
	const float packed[6] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, packed);
	glDrawArrays(GL_LINES, 0, 2);
	if (gl.lastGeometry().vertices.size() == 2)
		ctx.checkEqualBits(gl.lastGeometry().vertices[1].x, 4.0f, "stride zero packs tightly");
}

HEADLESS_TEST(legacygl_primitives, disabled_vertex_array_draws_nothing)
{
	legacygl::Context &gl = legacyglTest::begin();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	ctx.checkEqual(gl.drawCount(), 0, "no vertex array means no draw");
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "and no error either");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, nullptr);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "a null pointer with the array enabled is invalid");

	const float vertices[9] = { 0.0f };
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_TRIANGLES, -1, 3);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative first element is invalid");
	glVertexPointer(1, GL_FLOAT, 0, vertices);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a one-component position is invalid");
	glVertexPointer(3, GL_UNSIGNED_BYTE, 0, vertices);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unsupported position type is rejected");
}

HEADLESS_TEST(legacygl_primitives, immediate_mode_snapshots_attributes_per_vertex)
{
	legacygl::Context &gl = legacyglTest::begin();

	glBegin(GL_TRIANGLES);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glEnd();

	const legacygl::Geometry &drawn = gl.lastGeometry();
	ctx.checkEqual(static_cast<long long>(drawn.vertices.size()), 3, "three immediate vertices");
	if (drawn.vertices.size() == 3)
	{
		ctx.checkEqualBits(drawn.vertices[0].r, 1.0f, "the first vertex kept its own colour");
		ctx.checkEqualBits(drawn.vertices[1].g, 1.0f, "the second vertex kept its own colour");
		ctx.checkEqualBits(drawn.vertices[2].b, 1.0f, "the third vertex kept its own colour");
	}

	const legacygl::PrimitiveBatch &batch = gl.lastPrimitives();
	ctx.checkEqual(static_cast<long long>(batch.primitives.size()), 1, "one triangle was assembled");
	if (!batch.primitives.empty())
		ctx.checkEqual(batch.primitives[0].provoking, 2, "the flat colour comes from the third vertex");
}

HEADLESS_TEST(legacygl_primitives, current_attributes_survive_an_array_draw)
{
	legacygl::Context &gl = legacyglTest::begin();

	struct Vertex
	{
		float x, y, z;
		unsigned char rgba[4];
	};
	Vertex vertices[2];
	for (int i = 0; i < 2; i++)
	{
		vertices[i].x = static_cast<float>(i);
		vertices[i].y = 0.0f;
		vertices[i].z = 0.0f;
		vertices[i].rgba[0] = 10;
		vertices[i].rgba[1] = 20;
		vertices[i].rgba[2] = 30;
		vertices[i].rgba[3] = 40;
	}

	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
	const char *base = reinterpret_cast<const char *>(vertices);
	glVertexPointer(3, GL_FLOAT, 16, base);
	glColorPointer(4, GL_UNSIGNED_BYTE, 16, base + 12);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDrawArrays(GL_LINES, 0, 2);

	// OpenGL leaves the current colour undefined here. Measured against the
	// native driver it keeps the pre-draw value, and so does the core: no value
	// is invented, and the reliance is counted.
	ctx.checkEqualBits(gl.currentAttributes().r, 0.5f, "the pre-draw current colour is preserved");
	ctx.check(gl.currentColorIndeterminate(), "the core knows the value is formally indeterminate");

	glDisableClientState(GL_COLOR_ARRAY);
	const long long before = gl.indeterminateUseCount();
	glDrawArrays(GL_LINES, 0, 2);
	ctx.check(gl.indeterminateUseCount() > before, "leaning on the indeterminate colour is counted");

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	ctx.check(!gl.currentColorIndeterminate(), "an explicit setter makes the colour determinate again");
}
