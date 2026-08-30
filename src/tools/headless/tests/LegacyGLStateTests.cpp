// LegacyGL semantic state tests.
//
// Gate B of the parity plan: the frontend's state machine must behave like
// legacy OpenGL without a GPU in the loop. Every test drives the real gl* entry
// points and reads the semantic core back, so a regression here is a regression
// in what the backends are told to draw.

#include <limits>

#include "tools/headless/TestFramework.h"
#include "tools/headless/tests/LegacyGLFixture.h"

HEADLESS_TEST(legacygl_state, context_defaults_match_the_specification)
{
	legacygl::Context &gl = legacyglTest::begin();

	ctx.checkEqual(gl.currentMatrixMode(), GL_MODELVIEW, "initial matrix mode");
	ctx.check(gl.modelView().depth() == 1 && gl.projection().depth() == 1 && gl.textureMatrix().depth() == 1,
		"all three stacks start one deep");

	const float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	ctx.check(legacyglTest::matrixEquals(gl.modelView().top(), identity, 0.0f), "model-view starts as identity");
	ctx.check(legacyglTest::matrixEquals(gl.projection().top(), identity, 0.0f), "projection starts as identity");
	ctx.check(legacyglTest::matrixEquals(gl.textureMatrix().top(), identity, 0.0f), "texture starts as identity");

	const legacygl::Vertex &current = gl.currentAttributes();
	ctx.check(current.r == 1.0f && current.g == 1.0f && current.b == 1.0f && current.a == 1.0f,
		"current colour starts white and opaque");
	ctx.check(current.nx == 0.0f && current.ny == 0.0f && current.nz == 1.0f, "current normal starts (0,0,1)");
	ctx.check(current.s == 0.0f && current.t == 0.0f, "current texture coordinate starts (0,0)");

	ctx.check(!gl.isEnabled(GL_TEXTURE_2D), "texturing starts disabled");
	ctx.check(!gl.isEnabled(GL_DEPTH_TEST), "depth test starts disabled");
	ctx.check(!gl.isEnabled(GL_BLEND), "blending starts disabled");
	ctx.check(!gl.isEnabled(GL_ALPHA_TEST), "alpha test starts disabled");
	ctx.check(!gl.isEnabled(GL_CULL_FACE), "culling starts disabled");
	ctx.check(!gl.isEnabled(GL_FOG), "fog starts disabled");
	ctx.check(!gl.isEnabled(GL_LIGHTING), "lighting starts disabled");
	ctx.check(!gl.isEnabled(GL_LIGHT0), "light zero starts disabled");
	ctx.check(!gl.isEnabled(GL_COLOR_MATERIAL), "colour material starts disabled");
	ctx.check(!gl.isEnabled(GL_RESCALE_NORMAL), "rescale normal starts disabled");
	ctx.check(!gl.isEnabled(GL_NORMALIZE), "normalize starts disabled");
	ctx.check(!gl.isEnabled(GL_COLOR_LOGIC_OP), "colour logic op starts disabled");
	ctx.check(!gl.isEnabled(GL_POLYGON_OFFSET_FILL), "polygon offset fill starts disabled");
	ctx.check(!gl.isEnabled(GL_SCISSOR_TEST), "scissor test starts disabled");
	// Legacy OpenGL enables dithering by default. Getting this wrong makes
	// every pixel comparison unexplainable.
	ctx.check(gl.isEnabled(GL_DITHER), "dithering starts enabled");

	ctx.checkEqual(gl.blendSource(), GL_ONE, "default blend source factor");
	ctx.checkEqual(gl.blendDestination(), GL_ZERO, "default blend destination factor");
	ctx.checkEqual(gl.alphaTestFunc(), GL_ALWAYS, "default alpha function");
	ctx.checkEqualBits(gl.alphaTestRef(), 0.0f, "default alpha reference");
	ctx.checkEqual(gl.depthTestFunc(), GL_LESS, "default depth function");
	ctx.check(gl.depthWriteEnabled(), "depth writes start enabled");
	ctx.check(gl.colorWriteMask()[0] && gl.colorWriteMask()[1] && gl.colorWriteMask()[2] &&
			gl.colorWriteMask()[3],
		"colour mask starts fully enabled");
	ctx.checkEqual(gl.cullFaceMode(), GL_BACK, "default cull face");
	ctx.checkEqual(gl.frontFaceMode(), GL_CCW, "default front face");
	ctx.checkEqual(gl.shadeModelMode(), GL_SMOOTH, "default shade model");
	ctx.checkEqual(gl.logicOpcode(), GL_COPY, "default logic opcode");
	ctx.checkEqualBits(gl.lineWidthValue(), 1.0f, "default line width");
	ctx.checkEqualBits(gl.polygonOffsetFactor(), 0.0f, "default polygon offset factor");
	ctx.checkEqualBits(gl.polygonOffsetUnits(), 0.0f, "default polygon offset units");
	ctx.checkEqual(gl.packAlignment(), 4, "default pack alignment");
	ctx.checkEqual(gl.unpackAlignment(), 4, "default unpack alignment");

	ctx.checkEqual(gl.fogMode(), GL_EXP, "default fog mode");
	ctx.checkEqualBits(gl.fogDensity(), 1.0f, "default fog density");
	ctx.checkEqualBits(gl.fogStart(), 0.0f, "default fog start");
	ctx.checkEqualBits(gl.fogEnd(), 1.0f, "default fog end");
	ctx.check(gl.fogColor()[0] == 0.0f && gl.fogColor()[3] == 0.0f, "default fog colour is transparent black");
	ctx.check(!gl.fogUsesRadialDistance(), "fog distance starts at the eye-plane mode, not radial");

	// Light zero's diffuse and specular default to white; every other light
	// defaults to black. Collapsing that difference makes lit entities wrong.
	ctx.checkEqualBits(gl.light(0).diffuse[0], 1.0f, "light zero diffuse defaults to white");
	ctx.checkEqualBits(gl.light(1).diffuse[0], 0.0f, "light one diffuse defaults to black");
	ctx.checkEqualBits(gl.lightModelAmbient()[0], 0.2f, "default global ambient");
	ctx.checkEqual(gl.colorMaterialFace(), GL_FRONT_AND_BACK, "default colour material face");
	ctx.checkEqual(gl.colorMaterialMode(), GL_AMBIENT_AND_DIFFUSE, "default colour material mode");
	ctx.checkEqualBits(gl.frontMaterial().ambient[0], 0.2f, "default front material ambient");
	ctx.checkEqualBits(gl.frontMaterial().diffuse[0], 0.8f, "default front material diffuse");

	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "a fresh context has no pending error");
}

HEADLESS_TEST(legacygl_state, transform_calls_postmultiply)
{
	legacygl::Context &gl = legacyglTest::begin();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(1.0f, 2.0f, 3.0f);
	glScalef(2.0f, 2.0f, 2.0f);

	// M = T * S. Premultiplying instead would scale the translation to 2,4,6.
	const float expected[16] = { 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 1, 2, 3, 1 };
	ctx.check(legacyglTest::matrixEquals(gl.modelView().top(), expected, 0.0f),
		"translate then scale postmultiplies");

	glGetFloatv(GL_MODELVIEW_MATRIX, nullptr);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a null query destination is rejected");

	float readBack[16] = { 0.0f };
	glGetFloatv(GL_MODELVIEW_MATRIX, readBack);
	ctx.check(readBack[12] == 1.0f && readBack[13] == 2.0f && readBack[14] == 3.0f,
		"glGetFloatv returns the matrix the program built");
}

HEADLESS_TEST(legacygl_state, rotation_matches_the_axis_angle_definition)
{
	legacygl::Context &gl = legacyglTest::begin();

	glLoadIdentity();
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);

	// A 90 degree rotation about z maps x to y.
	const float expected[16] = { 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	ctx.check(legacyglTest::matrixEquals(gl.modelView().top(), expected, 1.0e-6f),
		"90 degrees about z rotates x onto y");

	glLoadIdentity();
	glRotatef(45.0f, 0.0f, 0.0f, 0.0f);
	const float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	ctx.check(legacyglTest::matrixEquals(gl.modelView().top(), identity, 0.0f),
		"a zero-length axis leaves the matrix alone instead of producing NaNs");

	// GameRenderer's hurt tilt divides by a hurtDuration that is zero until the
	// player has been damaged, so a frame rendered at a partial tick of exactly
	// zero asks for a NaN rotation. The reference driver's matrix is unchanged
	// by that call; propagating the NaN would poison the model-view and make
	// frustum culling reject the entire world.
	glLoadIdentity();
	glTranslatef(4.0f, 5.0f, 6.0f);
	glRotatef(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 1.0f);
	ctx.checkEqualBits(gl.modelView().top().m[12], 4.0f, "a NaN angle leaves the matrix untouched");
	ctx.checkEqualBits(gl.modelView().top().m[0], 1.0f, "and does not poison the rotation part");

	glRotatef(90.0f, std::numeric_limits<float>::infinity(), 0.0f, 0.0f);
	ctx.checkEqualBits(gl.modelView().top().m[13], 5.0f, "an infinite axis leaves the matrix untouched");
}

HEADLESS_TEST(legacygl_state, matrix_stack_overflow_and_underflow_are_reported)
{
	legacygl::Context &gl = legacyglTest::begin();

	glMatrixMode(GL_MODELVIEW);
	for (std::size_t i = 0; i + 1 < legacygl::Context::MODELVIEW_STACK_DEPTH; i++)
		glPushMatrix();
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "pushing to the stack limit is legal");
	ctx.checkEqual(static_cast<long long>(gl.modelView().depth()),
		static_cast<long long>(legacygl::Context::MODELVIEW_STACK_DEPTH), "stack is full");

	glPushMatrix();
	ctx.checkEqual(gl.getError(), GL_STACK_OVERFLOW, "one push too many overflows");
	ctx.checkEqual(static_cast<long long>(gl.modelView().depth()),
		static_cast<long long>(legacygl::Context::MODELVIEW_STACK_DEPTH),
		"an overflowing push does not change the stack");

	for (std::size_t i = 0; i + 1 < legacygl::Context::MODELVIEW_STACK_DEPTH; i++)
		glPopMatrix();
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "unwinding the stack is legal");

	glPopMatrix();
	ctx.checkEqual(gl.getError(), GL_STACK_UNDERFLOW, "popping an empty stack underflows");
}

HEADLESS_TEST(legacygl_state, matrix_modes_address_independent_stacks)
{
	legacygl::Context &gl = legacyglTest::begin();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(5.0f, 0.0f, 0.0f);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.0f, 7.0f, 0.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	ctx.checkEqualBits(gl.projection().top().m[12], 5.0f, "projection kept its own translation");
	ctx.checkEqualBits(gl.textureMatrix().top().m[13], 7.0f, "texture kept its own translation");
	ctx.checkEqualBits(gl.modelView().top().m[12], 0.0f, "model-view was untouched");

	glMatrixMode(GL_LIGHT0);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown matrix mode is rejected");
	ctx.checkEqual(gl.currentMatrixMode(), GL_MODELVIEW, "a rejected matrix mode does not take effect");
}

HEADLESS_TEST(legacygl_state, color3f_sets_alpha_to_one)
{
	legacygl::Context &gl = legacyglTest::begin();

	glColor4f(0.1f, 0.2f, 0.3f, 0.4f);
	ctx.checkEqualBits(gl.currentAttributes().a, 0.4f, "glColor4f sets alpha");

	glColor3f(0.5f, 0.6f, 0.7f);
	ctx.checkEqualBits(gl.currentAttributes().r, 0.5f, "glColor3f sets red");
	// It does not preserve the previous alpha.
	ctx.checkEqualBits(gl.currentAttributes().a, 1.0f, "glColor3f resets alpha to one");
}

HEADLESS_TEST(legacygl_state, normal3b_uses_the_legacy_normalization_rule)
{
	legacygl::Context &gl = legacyglTest::begin();

	glNormal3b(127, 0, -128);

	// OpenGL 1.1 table 2.6: (2c+1)/255, not c/127.
	ctx.checkEqualBits(gl.currentAttributes().nx, 1.0f, "127 normalizes to one");
	ctx.checkEqualBits(gl.currentAttributes().ny, 1.0f / 255.0f, "zero normalizes to 1/255");
	ctx.checkEqualBits(gl.currentAttributes().nz, -1.0f, "-128 normalizes to minus one");
}

HEADLESS_TEST(legacygl_state, alpha_reference_is_clamped_when_set)
{
	legacygl::Context &gl = legacyglTest::begin();

	glAlphaFunc(GL_GREATER, 2.5f);
	ctx.checkEqual(gl.alphaTestFunc(), GL_GREATER, "alpha function is stored");
	ctx.checkEqualBits(gl.alphaTestRef(), 1.0f, "an above-range reference clamps to one");

	glAlphaFunc(GL_LESS, -3.0f);
	ctx.checkEqualBits(gl.alphaTestRef(), 0.0f, "a below-range reference clamps to zero");

	glAlphaFunc(GL_TEXTURE_2D, 0.5f);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown comparison is rejected");
	ctx.checkEqual(gl.alphaTestFunc(), GL_LESS, "a rejected comparison does not take effect");
}

HEADLESS_TEST(legacygl_state, every_alpha_comparison_is_accepted)
{
	legacygl::Context &gl = legacyglTest::begin();

	const GLenum functions[8] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL,
		GL_ALWAYS };
	for (GLenum function : functions)
	{
		glAlphaFunc(function, 0.5f);
		ctx.checkEqual(gl.alphaTestFunc(), function, "alpha comparison is stored");
	}
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "no standard comparison is rejected");
}

HEADLESS_TEST(legacygl_state, unsupported_enable_is_rejected_not_ignored)
{
	legacygl::Context &gl = legacyglTest::begin();

	// GL_STENCIL_TEST is inside the tracked profile; GL_LINE_STIPPLE is not.
	glEnable(0x0B24);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an untracked capability is rejected");

	glEnable(GL_TEXTURE_2D);
	ctx.check(gl.isEnabled(GL_TEXTURE_2D), "a tracked capability is enabled");
	glDisable(GL_TEXTURE_2D);
	ctx.check(!gl.isEnabled(GL_TEXTURE_2D), "a tracked capability is disabled again");

	// Both spellings of rescale normal are the same state.
	glEnable(GL_RESCALE_NORMAL_EXT);
	ctx.check(gl.isEnabled(GL_RESCALE_NORMAL), "GL_RESCALE_NORMAL_EXT is the same bit as GL_RESCALE_NORMAL");
	// And it is not the same state as GL_NORMALIZE.
	ctx.check(!gl.isEnabled(GL_NORMALIZE), "rescale normal is not normalize");
}

HEADLESS_TEST(legacygl_state, first_error_is_latched_until_read)
{
	legacygl::Context &gl = legacyglTest::begin();

	glCullFace(GL_LIGHT0);
	glShadeModel(GL_LIGHT0);
	ctx.checkEqual(gl.pendingError(), GL_INVALID_ENUM, "the first error is held");

	// glGetError returns the first pending error and clears it.
	ctx.checkEqual(glGetError(), GL_INVALID_ENUM, "glGetError returns the latched error");
	ctx.checkEqual(glGetError(), GL_NO_ERROR, "reading the error clears it");

	glLineWidth(0.0f);
	ctx.checkEqual(glGetError(), GL_INVALID_VALUE, "a non-positive line width is invalid");
	glViewport(0, 0, -1, 4);
	ctx.checkEqual(glGetError(), GL_INVALID_VALUE, "a negative viewport size is invalid");
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA_SATURATE);
	ctx.checkEqual(glGetError(), GL_INVALID_ENUM, "saturate is not a destination factor");
}

HEADLESS_TEST(legacygl_state, light_position_is_transformed_when_the_setter_runs)
{
	legacygl::Context &gl = legacyglTest::begin();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 5.0f, 0.0f);

	const GLfloat position[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	ctx.checkEqualBits(gl.light(0).positionEye[1], 5.0f, "the position was transformed at call time");
	ctx.checkEqualBits(gl.light(0).positionEye[3], 1.0f, "a positional light keeps w = 1");

	// Changing the matrix afterwards must not move the light.
	glTranslatef(0.0f, 100.0f, 0.0f);
	ctx.checkEqualBits(gl.light(0).positionEye[1], 5.0f, "a later matrix change does not move the light");

	// A directional light keeps w = 0 and is rotated, not translated.
	glLoadIdentity();
	glTranslatef(9.0f, 9.0f, 9.0f);
	const GLfloat direction[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	glLightfv(GL_LIGHT1, GL_POSITION, direction);
	ctx.checkEqualBits(gl.light(1).positionEye[0], 0.0f, "translation does not affect a directional light");
	ctx.checkEqualBits(gl.light(1).positionEye[1], 1.0f, "the direction survives");
	ctx.checkEqualBits(gl.light(1).positionEye[3], 0.0f, "a directional light keeps w = 0");
}

HEADLESS_TEST(legacygl_state, color_material_tracks_current_colour_and_persists)
{
	legacygl::Context &gl = legacyglTest::begin();

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT);
	glColor4f(0.25f, 0.5f, 0.75f, 1.0f);

	ctx.checkEqualBits(gl.frontMaterial().ambient[0], 0.25f, "front ambient followed the current colour");
	ctx.checkEqualBits(gl.frontMaterial().diffuse[0], 0.8f, "front diffuse was not selected");
	ctx.checkEqualBits(gl.backMaterial().ambient[0], 0.2f, "the back face was not selected");

	// Disabling colour material keeps the values it wrote.
	glDisable(GL_COLOR_MATERIAL);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	ctx.checkEqualBits(gl.frontMaterial().ambient[0], 0.25f, "tracked material values persist");

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
	ctx.checkEqualBits(gl.frontMaterial().ambient[0], 0.5f, "ambient and diffuse tracks ambient");
	ctx.checkEqualBits(gl.frontMaterial().diffuse[0], 0.5f, "ambient and diffuse tracks diffuse");
	ctx.checkEqualBits(gl.backMaterial().diffuse[0], 0.5f, "front and back tracks both faces");

	glColorMaterial(GL_FRONT, GL_LIGHT0);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown colour material mode is rejected");
}

HEADLESS_TEST(legacygl_state, fog_state_and_the_nv_distance_mode_are_separate)
{
	legacygl::Context &gl = legacyglTest::begin();

	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogf(GL_FOG_DENSITY, 0.25f);
	glFogf(GL_FOG_START, 10.0f);
	glFogf(GL_FOG_END, 90.0f);
	const GLfloat color[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
	glFogfv(GL_FOG_COLOR, color);

	ctx.checkEqual(gl.fogMode(), GL_EXP2, "fog mode is stored");
	ctx.checkEqualBits(gl.fogDensity(), 0.25f, "fog density is stored");
	ctx.checkEqualBits(gl.fogStart(), 10.0f, "fog start is stored");
	ctx.checkEqualBits(gl.fogEnd(), 90.0f, "fog end is stored");
	ctx.checkEqualBits(gl.fogColor()[1], 0.2f, "fog colour is stored");
	ctx.check(!gl.fogUsesRadialDistance(), "setting the fog mode does not select radial distance");

	// The nether path asks for radial eye distance explicitly. That is a
	// different concept from the fog equation and has its own state.
	glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
	ctx.check(gl.fogUsesRadialDistance(), "the NV distance mode selects radial distance");
	ctx.checkEqual(gl.fogMode(), GL_EXP2, "the fog equation is unchanged by the distance mode");

	glFogi(GL_FOG_MODE, GL_NEAREST);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown fog mode is rejected");
	ctx.checkEqual(gl.fogMode(), GL_EXP2, "a rejected fog mode does not take effect");

	glFogf(GL_FOG_DENSITY, -1.0f);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative fog density is invalid");
}

HEADLESS_TEST(legacygl_state, pixel_store_accepts_only_the_supported_alignments)
{
	legacygl::Context &gl = legacyglTest::begin();

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	ctx.checkEqual(gl.packAlignment(), 1, "pack alignment one is accepted");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	ctx.checkEqual(gl.unpackAlignment(), 8, "unpack alignment eight is accepted");

	glPixelStorei(GL_PACK_ALIGNMENT, 3);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a non-power-of-two alignment is invalid");
	ctx.checkEqual(gl.packAlignment(), 1, "a rejected alignment does not take effect");

	// Row length and skip controls are outside the supported profile and are
	// refused rather than silently ignored.
	glPixelStorei(0x0CF2, 4);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unimplemented pixel store control is rejected");
}

HEADLESS_TEST(legacygl_state, queries_are_answered_from_the_semantic_core)
{
	legacygl::Context &gl = legacyglTest::begin();

	glColor4f(0.2f, 0.4f, 0.6f, 0.8f);
	glNormal3f(1.0f, 0.0f, 0.0f);
	glLineWidth(2.0f);
	glPolygonOffset(-3.0f, -3.0f);
	glClearColor(0.1f, 0.2f, 0.3f, 0.4f);
	glViewport(1, 2, 3, 4);

	float color[4] = { 0.0f };
	glGetFloatv(GL_CURRENT_COLOR, color);
	ctx.check(color[0] == 0.2f && color[3] == 0.8f, "current colour query");

	float normal[3] = { 0.0f };
	glGetFloatv(GL_CURRENT_NORMAL, normal);
	ctx.check(normal[0] == 1.0f && normal[2] == 0.0f, "current normal query");

	float scalar = 0.0f;
	glGetFloatv(GL_LINE_WIDTH, &scalar);
	ctx.checkEqualBits(scalar, 2.0f, "line width query");
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &scalar);
	ctx.checkEqualBits(scalar, -3.0f, "polygon offset factor is kept in canonical GL units");

	float clear[4] = { 0.0f };
	glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
	ctx.check(clear[2] == 0.3f, "clear colour query");

	float viewport[4] = { 0.0f };
	glGetFloatv(GL_VIEWPORT, viewport);
	ctx.check(viewport[2] == 3.0f && viewport[3] == 4.0f, "viewport query");

	glGetFloatv(GL_LIGHT0, &scalar);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unsupported query name is rejected");
}

HEADLESS_TEST(legacygl_state, ortho_and_frustum_reject_degenerate_volumes)
{
	legacygl::Context &gl = legacyglTest::begin();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 0.0, 0.0, 480.0, 1000.0, 3000.0);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "an empty ortho volume is invalid");

	glOrtho(0.0, 854.0, 480.0, 0.0, 1000.0, 3000.0);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "the GUI ortho projection is accepted");
	// The GUI projection maps y = 0 to the top of the window, so the y scale is
	// negative. Getting the sign wrong flips every screen.
	ctx.check(gl.projection().top().m[5] < 0.0f, "the inverted ortho volume produces a negative y scale");

	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 0.0, 100.0);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a zero near plane is invalid");

	glFrustum(-1.0, 1.0, -1.0, 1.0, 0.05, 256.0);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "a normal frustum is accepted");
	ctx.checkEqualBits(gl.projection().top().m[11], -1.0f, "the perspective frustum keeps w = -z");
}

// The normal transform, the rescale-normal factor and non-uniform-scale
// detection are computed once in the core so that every backend inherits the
// same answer instead of each shader inventing one. They are separate
// behaviours: neither is "normalize everything".
HEADLESS_TEST(legacygl_state, normals_use_the_inverse_transpose_model_view)
{
	legacyglTest::begin();

	// A non-uniform scale is where multiplying a normal by the model-view goes
	// wrong: scaling x by 2 must scale the normal's x by 1/2.
	const legacygl::Mat4 modelView = legacygl::scaling(2.0f, 1.0f, 1.0f);
	const legacygl::Mat4 normal = legacygl::normalMatrix(modelView);

	ctx.checkEqualBits(normal.m[0], 0.5f, "the x axis is inverted, not applied");
	ctx.checkEqualBits(normal.m[5], 1.0f, "the unscaled axes are unchanged");
	ctx.checkEqualBits(normal.m[10], 1.0f, "and so is z");

	// A rotation is its own inverse transpose, so the normal matrix equals the
	// rotation.
	const legacygl::Mat4 rotated = legacygl::rotation(37.0f, 0.0f, 1.0f, 0.0f);
	const legacygl::Mat4 rotatedNormal = legacygl::normalMatrix(rotated);
	for (int i = 0; i < 16; i++)
	{
		const float difference = rotatedNormal.m[i] - rotated.m[i];
		if (difference > 1.0e-6f || difference < -1.0e-6f)
		{
			ctx.fail("a rotation should be its own inverse transpose");
			break;
		}
	}

	// A singular model-view must not produce NaN normals.
	const legacygl::Mat4 collapsed = legacygl::scaling(1.0f, 0.0f, 1.0f);
	ctx.checkEqualBits(legacygl::normalMatrix(collapsed).m[0], 1.0f,
		"a singular model-view falls back to the identity");
}

HEADLESS_TEST(legacygl_state, rescale_normal_factor_is_not_normalization)
{
	legacyglTest::begin();

	// GL_RESCALE_NORMAL restores unit length for a uniformly scaled model-view
	// with one scalar. For a uniform scale of 4 the inverse transpose scales
	// normals by 1/4, so the factor is 4.
	const legacygl::Mat4 uniform = legacygl::scaling(4.0f, 4.0f, 4.0f);
	ctx.checkEqualBits(legacygl::rescaleNormalFactor(uniform), 4.0f, "uniform scale of four gives a factor of four");
	ctx.checkEqualBits(legacygl::rescaleNormalFactorFromNormalMatrix(legacygl::normalMatrix(uniform)),
		legacygl::rescaleNormalFactor(uniform), "the precomputed normal matrix gives the same factor");
	ctx.check(legacygl::isUniformScale(uniform, 1.0e-5f), "a uniform scale is detected as uniform");

	const legacygl::Mat4 identity = legacygl::Mat4::identity();
	ctx.checkEqualBits(legacygl::rescaleNormalFactor(identity), 1.0f, "the identity needs no rescale");

	// Under a non-uniform scale the rescale guarantee no longer holds, which is
	// what the validation build needs to be able to say.
	const legacygl::Mat4 nonUniform = legacygl::scaling(2.0f, 1.0f, 1.0f);
	ctx.check(!legacygl::isUniformScale(nonUniform, 1.0e-5f), "a non-uniform scale is detected");

	// A rotation preserves lengths, so it is uniform and needs no rescale.
	const legacygl::Mat4 rotated = legacygl::rotation(45.0f, 1.0f, 0.0f, 0.0f);
	ctx.check(legacygl::isUniformScale(rotated, 1.0e-5f), "a rotation is a uniform transform");
	const float factor = legacygl::rescaleNormalFactor(rotated);
	ctx.check(factor > 0.999f && factor < 1.001f, "a rotation's rescale factor is one");
}
