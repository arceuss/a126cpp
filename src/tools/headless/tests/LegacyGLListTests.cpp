// LegacyGL display-list semantics tests.
//
// Alpha builds font glyphs, chunk geometry, model cubes, the sky and cached
// signs out of display lists, so these rules are load-bearing:
//
//   - GL_COMPILE records without executing; GL_COMPILE_AND_EXECUTE does both.
//   - Client-array state and pointer setters are never compiled.
//   - A compiled array draw captures the vertex data at compile time.
//   - Immediate-mode vertices read the current attributes the list installs
//     while it executes, not the ones that happened to be current while it was
//     compiled.
//
// The last rule is what lets a cached sign list carry its own glColor3f and a
// glyph list be recoloured from outside.

#include <vector>

#include "tools/headless/TestFramework.h"
#include "tools/headless/tests/LegacyGLFixture.h"

HEADLESS_TEST(legacygl_lists, generation_reserves_names_without_defining_them)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint base = glGenLists(4);
	ctx.check(base != 0, "glGenLists returned a name");
	ctx.check(gl.displayList(base) == nullptr || !gl.displayList(base)->defined,
		"a generated name is not a defined list");

	// Calling an undefined list draws nothing and raises no error.
	glCallList(base);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "calling an undefined list is not an error");
	ctx.checkEqual(gl.drawCount(), 0, "an undefined list draws nothing");

	ctx.checkEqual(static_cast<long long>(glGenLists(0)), 0, "a zero range returns zero");
	glGenLists(-1);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative range is invalid");
}

HEADLESS_TEST(legacygl_lists, compile_records_without_executing)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint list = glGenLists(1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glNewList(list, GL_COMPILE);
	glTranslatef(3.0f, 0.0f, 0.0f);
	glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
	glEndList();

	// Nothing in the list ran.
	ctx.checkEqualBits(gl.modelView().top().m[12], 0.0f, "GL_COMPILE did not apply the translation");
	ctx.checkEqualBits(gl.currentAttributes().a, 1.0f, "GL_COMPILE did not apply the colour");
	ctx.check(gl.displayList(list) != nullptr && gl.displayList(list)->defined, "the list is defined");

	glCallList(list);
	ctx.checkEqualBits(gl.modelView().top().m[12], 3.0f, "calling the list applied the translation");
	ctx.checkEqualBits(gl.currentAttributes().a, 0.5f, "calling the list applied the colour");

	// Calling it again accumulates, proving the commands are replayed rather
	// than a final state being snapshotted.
	glCallList(list);
	ctx.checkEqualBits(gl.modelView().top().m[12], 6.0f, "a second call applies the translation again");
}

HEADLESS_TEST(legacygl_lists, compile_and_execute_does_both)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint list = glGenLists(1);
	glLoadIdentity();

	glNewList(list, GL_COMPILE_AND_EXECUTE);
	glTranslatef(0.0f, 2.0f, 0.0f);
	glEndList();

	ctx.checkEqualBits(gl.modelView().top().m[13], 2.0f, "the command executed while compiling");
	glCallList(list);
	ctx.checkEqualBits(gl.modelView().top().m[13], 4.0f, "and it was recorded as well");
}

HEADLESS_TEST(legacygl_lists, nesting_a_new_list_is_an_error)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint base = glGenLists(2);
	glNewList(base, GL_COMPILE);
	glNewList(base + 1, GL_COMPILE);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "glNewList inside a list is invalid");
	ctx.checkEqual(static_cast<long long>(gl.compilingListNameValue()), static_cast<long long>(base),
		"the original list is still compiling");
	glEndList();

	glEndList();
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "glEndList without a list is invalid");

	glNewList(0, GL_COMPILE);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "list zero cannot be compiled");
	glNewList(base, GL_NEAREST);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown compile mode is rejected");
}

HEADLESS_TEST(legacygl_lists, client_state_is_not_compiled_into_a_list)
{
	legacygl::Context &gl = legacyglTest::begin();

	const float vertices[9] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	const GLuint list = glGenLists(1);

	glNewList(list, GL_COMPILE);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEndList();

	// Both commands took effect immediately even though a list was open, and
	// neither was recorded.
	ctx.check(gl.vertexArray().enabled, "glEnableClientState executed immediately");
	ctx.check(gl.vertexArray().pointer == vertices, "glVertexPointer executed immediately");

	glDisableClientState(GL_VERTEX_ARRAY);
	glCallList(list);
	ctx.check(!gl.vertexArray().enabled, "the list did not re-enable the client array");
}

HEADLESS_TEST(legacygl_lists, compiled_array_draw_captures_its_vertices)
{
	legacygl::Context &gl = legacyglTest::begin();

	std::vector<float> vertices = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	const GLuint list = glGenLists(1);

	glVertexPointer(3, GL_FLOAT, 0, vertices.data());
	glEnableClientState(GL_VERTEX_ARRAY);
	glNewList(list, GL_COMPILE);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glEndList();
	glDisableClientState(GL_VERTEX_ARRAY);

	// Mutating and then destroying the source must not change what the list
	// draws: the data belongs to the list now.
	vertices[3] = 500.0f;
	vertices.clear();
	vertices.shrink_to_fit();

	glCallList(list);
	const legacygl::Geometry &drawn = gl.lastGeometry();
	ctx.checkEqual(static_cast<long long>(drawn.vertices.size()), 3, "the captured draw replayed three vertices");
	if (drawn.vertices.size() == 3)
	{
		ctx.checkEqualBits(drawn.vertices[1].x, 1.0f, "the captured vertex kept its compile-time value");
		ctx.checkEqualBits(drawn.vertices[2].y, 1.0f, "the third captured vertex survived");
	}
}

HEADLESS_TEST(legacygl_lists, captured_draw_takes_unsupplied_attributes_at_execution)
{
	legacygl::Context &gl = legacyglTest::begin();

	// This is the font glyph case: the list carries positions and texture
	// coordinates but no colours, so the colour comes from whatever glColor4f
	// ran before the call.
	const float interleaved[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	const GLuint list = glGenLists(1);

	glVertexPointer(3, GL_FLOAT, 16, interleaved);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNewList(list, GL_COMPILE);
	glDrawArrays(GL_LINES, 0, 2);
	glEndList();
	glDisableClientState(GL_VERTEX_ARRAY);

	glColor4f(0.25f, 0.5f, 0.75f, 1.0f);
	glCallList(list);
	ctx.check(!gl.lastGeometry().hasColor, "the captured draw supplied no colour");
	if (!gl.lastGeometry().vertices.empty())
	{
		ctx.checkEqualBits(gl.lastGeometry().vertices[0].r, 0.25f, "the colour came from the call site");
		ctx.checkEqualBits(gl.lastGeometry().vertices[0].b, 0.75f, "and so did the blue channel");
	}

	// A different surrounding colour recolours the same list.
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glCallList(list);
	if (!gl.lastGeometry().vertices.empty())
		ctx.checkEqualBits(gl.lastGeometry().vertices[0].g, 1.0f, "the list is recolourable");
}

HEADLESS_TEST(legacygl_lists, immediate_vertices_read_the_lists_own_colour)
{
	legacygl::Context &gl = legacyglTest::begin();

	// The cached world sign compiles glColor3f and then immediate-mode
	// geometry. Under GL_COMPILE the colour command has not run when the
	// vertices are compiled, so capturing the current colour at compile time
	// would bake in whatever the renderer happened to be using.
	const GLuint list = glGenLists(1);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNewList(list, GL_COMPILE);
	glColor3f(0.125f, 0.25f, 0.375f);
	glBegin(GL_TRIANGLES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();
	glEndList();

	ctx.checkEqualBits(gl.currentAttributes().r, 1.0f, "compiling did not change the current colour");
	ctx.checkEqual(gl.drawCount(), 0, "compiling did not draw");

	glCallList(list);
	ctx.checkEqual(gl.drawCount(), 1, "executing the list drew once");
	const legacygl::Geometry &drawn = gl.lastGeometry();
	ctx.checkEqual(static_cast<long long>(drawn.vertices.size()), 3, "three immediate vertices replayed");
	if (drawn.vertices.size() == 3)
	{
		ctx.checkEqualBits(drawn.vertices[0].r, 0.125f, "the vertex used the list's own colour");
		ctx.checkEqualBits(drawn.vertices[2].b, 0.375f, "every vertex used it");
	}
	ctx.checkEqualBits(gl.currentAttributes().r, 0.125f, "the list's colour command is still in effect");
}

HEADLESS_TEST(legacygl_lists, immediate_mode_outside_begin_is_an_error)
{
	legacygl::Context &gl = legacyglTest::begin();

	glVertex3f(0.0f, 0.0f, 0.0f);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "a vertex outside glBegin is invalid");
	glEnd();
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "glEnd without glBegin is invalid");

	glBegin(GL_TRIANGLES);
	glBegin(GL_TRIANGLES);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "nested glBegin is invalid");
	glDrawArrays(GL_TRIANGLES, 0, 3);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "an array draw inside glBegin is invalid");
	glEnd();

	glBegin(GL_LINE_STRIP + 100);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown primitive mode is rejected");
}

HEADLESS_TEST(legacygl_lists, nested_calls_preserve_command_order)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint base = glGenLists(2);
	const GLuint inner = base;
	const GLuint outer = base + 1;

	glNewList(inner, GL_COMPILE);
	glScalef(2.0f, 2.0f, 2.0f);
	glEndList();

	glNewList(outer, GL_COMPILE);
	glTranslatef(1.0f, 0.0f, 0.0f);
	glCallList(inner);
	glTranslatef(1.0f, 0.0f, 0.0f);
	glEndList();

	glLoadIdentity();
	glCallList(outer);

	// translate(1) * scale(2) * translate(1) puts the second translation in
	// scaled space, so x ends at 1 + 2 = 3. A reordered replay gives 2 or 4.
	ctx.checkEqualBits(gl.modelView().top().m[12], 3.0f, "nested commands ran in order");
	ctx.checkEqualBits(gl.modelView().top().m[0], 2.0f, "the nested scale applied");
}

HEADLESS_TEST(legacygl_lists, call_lists_decodes_unsigned_int_names)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint base = glGenLists(3);
	for (GLuint i = 0; i < 3; i++)
	{
		glNewList(base + i, GL_COMPILE);
		glTranslatef(1.0f, 0.0f, 0.0f);
		glEndList();
	}

	// The font path accumulates unsigned int glyph names and submits them in
	// one call.
	const GLuint names[4] = { base, base + 1, base + 2, base };
	glLoadIdentity();
	glCallLists(4, GL_UNSIGNED_INT, names);
	ctx.checkEqualBits(gl.modelView().top().m[12], 4.0f, "every named list ran once");

	const unsigned char byteNames[2] = { 1, 2 };
	glCallLists(2, GL_UNSIGNED_BYTE, byteNames);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "unsigned byte names are accepted");

	// The 2_BYTES/3_BYTES/4_BYTES encodings are outside the supported profile.
	glCallLists(1, 0x1407, names);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unsupported element type is rejected");
	glCallLists(-1, GL_UNSIGNED_INT, names);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative count is invalid");
}

HEADLESS_TEST(legacygl_lists, redefinition_replaces_the_previous_contents)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint list = glGenLists(1);

	glNewList(list, GL_COMPILE);
	glTranslatef(10.0f, 0.0f, 0.0f);
	glEndList();

	glNewList(list, GL_COMPILE);
	glTranslatef(1.0f, 0.0f, 0.0f);
	glEndList();

	glLoadIdentity();
	glCallList(list);
	ctx.checkEqualBits(gl.modelView().top().m[12], 1.0f, "redefinition dropped the old commands");

	glDeleteLists(list, 1);
	ctx.check(gl.displayList(list) == nullptr, "the definition is gone");
	glLoadIdentity();
	glCallList(list);
	ctx.checkEqualBits(gl.modelView().top().m[12], 0.0f, "a deleted list draws nothing");
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "calling a deleted list is not an error");

	glDeleteLists(list, -1);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative delete range is invalid");
}

HEADLESS_TEST(legacygl_lists, a_list_that_calls_itself_stops_at_the_nesting_limit)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glCallList(list);
	glEndList();

	glCallList(list);
	// Unbounded recursion would blow the stack; the frontend reports the
	// nesting limit instead.
	ctx.checkEqual(gl.getError(), GL_STACK_OVERFLOW, "self recursion is stopped and reported");
	ctx.checkEqual(gl.listDepth(), 0, "the execution depth unwound");
}

HEADLESS_TEST(legacygl_lists, texture_binds_in_a_list_resolve_at_execution)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint texture = 0;
	glGenTextures(1, &texture);
	const GLuint list = glGenLists(1);

	glNewList(list, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEndList();

	ctx.checkEqual(static_cast<long long>(gl.boundTexture()), 0, "compiling did not bind");
	glCallList(list);
	ctx.checkEqual(static_cast<long long>(gl.boundTexture()), static_cast<long long>(texture),
		"executing the list bound the texture");
	// The list stores the public name, so deleting and recreating the object
	// behind that name is visible to a later execution.
	ctx.check(gl.isTextureObject(texture), "executing the bind created the object");
}
