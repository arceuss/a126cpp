// LegacyGL backend dispatch tests.
//
// The frontend must hand every inventoried call to the active backend, once, in
// the order the game issued it. That is the whole basis of the native
// compatibility path being usable as an oracle, and it is invisible to the state
// tests: the semantic core can be perfectly correct while the backend receives
// nothing and the screen stays empty.
//
// A recording backend makes the dispatch observable, so a dropped forward fails
// here instead of only in a screenshot.

#include <string>
#include <vector>

#include "legacygl/Sink.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/tests/LegacyGLFixture.h"

class RecordingSink : public legacygl::Sink
{
public:
	std::vector<std::string> calls;
	std::vector<std::string> resolvedCalls;
	std::vector<legacygl::ResolvedDraw> resolvedDraws;
	std::vector<legacygl::Geometry> resolvedGeometry;
	std::vector<legacygl::PrimitiveBatch> resolvedPrimitives;
	std::vector<legacygl::ResolvedClear> resolvedClears;
	std::vector<legacygl::ResolvedTextureUpload> resolvedUploads;
	std::vector<legacygl::ResolvedReadback> resolvedReadbacks;

	void matrixMode(unsigned int) override { calls.push_back("matrixMode"); }
	void loadIdentity() override { calls.push_back("loadIdentity"); }
	void pushMatrix() override { calls.push_back("pushMatrix"); }
	void popMatrix() override { calls.push_back("popMatrix"); }
	void translatef(float, float, float) override { calls.push_back("translatef"); }
	void rotatef(float, float, float, float) override { calls.push_back("rotatef"); }
	void scalef(float, float, float) override { calls.push_back("scalef"); }
	void scaled(double, double, double) override { calls.push_back("scaled"); }
	void ortho(double, double, double, double, double, double) override { calls.push_back("ortho"); }
	void frustum(double, double, double, double, double, double) override { calls.push_back("frustum"); }

	void enable(unsigned int) override { calls.push_back("enable"); }
	void disable(unsigned int) override { calls.push_back("disable"); }
	void blendFunc(unsigned int, unsigned int) override { calls.push_back("blendFunc"); }
	void alphaFunc(unsigned int, float) override { calls.push_back("alphaFunc"); }
	void depthFunc(unsigned int) override { calls.push_back("depthFunc"); }
	void depthMask(unsigned char) override { calls.push_back("depthMask"); }
	void colorMask(unsigned char, unsigned char, unsigned char, unsigned char) override
	{
		calls.push_back("colorMask");
	}
	void cullFace(unsigned int) override { calls.push_back("cullFace"); }
	void shadeModel(unsigned int) override { calls.push_back("shadeModel"); }
	void logicOp(unsigned int) override { calls.push_back("logicOp"); }
	void lineWidth(float) override { calls.push_back("lineWidth"); }
	void polygonOffset(float, float) override { calls.push_back("polygonOffset"); }
	void viewport(int, int, int, int) override { calls.push_back("viewport"); }
	void pixelStorei(unsigned int, int) override { calls.push_back("pixelStorei"); }

	void color4f(float, float, float, float) override { calls.push_back("color4f"); }
	void color3f(float, float, float) override { calls.push_back("color3f"); }
	void normal3f(float, float, float) override { calls.push_back("normal3f"); }
	void normal3b(signed char, signed char, signed char) override { calls.push_back("normal3b"); }

	void fogf(unsigned int, float) override { calls.push_back("fogf"); }
	void fogfv(unsigned int, const float *) override { calls.push_back("fogfv"); }
	void fogi(unsigned int, int) override { calls.push_back("fogi"); }

	void lightfv(unsigned int, unsigned int, const float *) override { calls.push_back("lightfv"); }
	void lightModelfv(unsigned int, const float *) override { calls.push_back("lightModelfv"); }
	void colorMaterial(unsigned int, unsigned int) override { calls.push_back("colorMaterial"); }

	void genTextures(int n, unsigned int *textures) override
	{
		calls.push_back("genTextures");
		for (int i = 0; i < n; i++)
			textures[i] = nextName++;
	}
	void deleteTextures(int, const unsigned int *) override { calls.push_back("deleteTextures"); }
	void bindTexture(unsigned int, unsigned int) override { calls.push_back("bindTexture"); }
	void texParameteri(unsigned int, unsigned int, int) override { calls.push_back("texParameteri"); }
	void texImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *) override
	{
		calls.push_back("texImage2D");
	}
	void texSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *) override
	{
		calls.push_back("texSubImage2D");
	}

	void enableClientState(unsigned int) override { calls.push_back("enableClientState"); }
	void disableClientState(unsigned int) override { calls.push_back("disableClientState"); }
	void vertexPointer(int, unsigned int, int, const void *) override { calls.push_back("vertexPointer"); }
	void texCoordPointer(int, unsigned int, int, const void *) override { calls.push_back("texCoordPointer"); }
	void colorPointer(int, unsigned int, int, const void *) override { calls.push_back("colorPointer"); }
	void normalPointer(unsigned int, int, const void *) override { calls.push_back("normalPointer"); }
	void drawArrays(unsigned int, int, int) override { calls.push_back("drawArrays"); }

	void begin(unsigned int) override { calls.push_back("begin"); }
	void end() override { calls.push_back("end"); }
	void vertex3f(float, float, float) override { calls.push_back("vertex3f"); }
	void texCoord2f(float, float) override { calls.push_back("texCoord2f"); }

	void genBuffersARB(int n, unsigned int *buffers) override
	{
		calls.push_back("genBuffersARB");
		for (int i = 0; i < n; i++)
			buffers[i] = nextName++;
	}
	void bindBufferARB(unsigned int, unsigned int) override { calls.push_back("bindBufferARB"); }
	void bufferDataARB(unsigned int, std::ptrdiff_t, const void *, unsigned int) override
	{
		calls.push_back("bufferDataARB");
	}

	unsigned int genLists(int range) override
	{
		calls.push_back("genLists");
		const unsigned int base = nextName;
		nextName += static_cast<unsigned int>(range > 0 ? range : 0);
		return base;
	}
	void newList(unsigned int, unsigned int) override { calls.push_back("newList"); }
	void endList() override { calls.push_back("endList"); }
	void callList(unsigned int) override { calls.push_back("callList"); }
	void callLists(int, unsigned int, const void *) override { calls.push_back("callLists"); }
	void deleteLists(unsigned int, int) override { calls.push_back("deleteLists"); }

	void clear(unsigned int) override { calls.push_back("clear"); }
	void clearColor(float, float, float, float) override { calls.push_back("clearColor"); }
	void clearDepth(double) override { calls.push_back("clearDepth"); }
	void readPixels(int, int, int, int, unsigned int, unsigned int, void *) override
	{
		calls.push_back("readPixels");
	}
	void finish() override { calls.push_back("finish"); }

	bool wantsCanonicalGeometry() const override { return true; }
	void resolvedDraw(const legacygl::ResolvedDraw &command) override
	{
		resolvedCalls.push_back("draw");
		resolvedDraws.push_back(command);
		resolvedGeometry.push_back(*command.geometry);
		resolvedPrimitives.push_back(*command.primitives);
		resolvedDraws.back().geometry = nullptr;
		resolvedDraws.back().primitives = nullptr;
	}
	void resolvedClear(const legacygl::ResolvedClear &command) override
	{
		resolvedCalls.push_back("clear");
		resolvedClears.push_back(command);
	}
	void resolvedTextureUpload(const legacygl::ResolvedTextureUpload &command) override
	{
		resolvedCalls.push_back("textureUpload");
		resolvedUploads.push_back(command);
	}
	void resolvedReadback(const legacygl::ResolvedReadback &command) override
	{
		resolvedCalls.push_back("readback");
		resolvedReadbacks.push_back(command);
	}

	long long countOf(const std::string &name) const
	{
		long long total = 0;
		for (const std::string &call : calls)
		{
			if (call == name)
				total++;
		}
		return total;
	}

private:
	unsigned int nextName = 1;
};

HEADLESS_TEST(legacygl_dispatch, every_inventoried_call_reaches_the_backend)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	// One call each, in the shape the renderer uses them.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(1.0f, 0.0f, 0.0f);
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
	glScalef(2.0f, 2.0f, 2.0f);
	glScaled(0.5, 0.5, 1.0);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 854.0, 480.0, 0.0, 1000.0, 3000.0);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 0.05, 256.0);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.1f);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glColorMask(true, true, true, false);
	glCullFace(GL_BACK);
	glShadeModel(GL_FLAT);
	glLogicOp(GL_OR_REVERSE);
	glLineWidth(2.0f);
	glPolygonOffset(-3.0f, -3.0f);
	glViewport(0, 0, 854, 480);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glColor3f(0.5f, 0.5f, 0.5f);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glNormal3b(0, 127, 0);

	glFogf(GL_FOG_DENSITY, 0.1f);
	const GLfloat fogColor[4] = { 0.5f, 0.6f, 0.7f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_MODE, GL_EXP);

	const GLfloat lightValue[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightValue);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightValue);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	std::vector<unsigned char> pixels(16 * 16 * 4, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glDeleteTextures(1, &texture);

	// The Tesselator submission shape.
	const float vertices[9] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 12, vertices);
	glColorPointer(4, GL_UNSIGNED_BYTE, 12, vertices);
	glNormalPointer(GL_BYTE, 12, vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBegin(GL_TRIANGLES);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();

	GLuint buffer = 0;
	glGenBuffersARB(1, &buffer);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(sizeof(vertices)), vertices,
		GL_STREAM_DRAW_ARB);

	const GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glTranslatef(0.0f, 1.0f, 0.0f);
	glEndList();
	glCallList(list);
	const GLuint names[1] = { list };
	glCallLists(1, GL_UNSIGNED_INT, names);
	glDeleteLists(list, 1);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	std::vector<unsigned char> readback(4 * 4 * 3, 0);
	glReadPixels(0, 0, 4, 4, GL_BGR_EXT, GL_UNSIGNED_BYTE, readback.data());
	glFinish();

	// Every entry point above must have reached the backend. A dropped forward
	// leaves the semantic core correct and the screen empty.
	const char *expectedOnce[] = { "scaled", "ortho", "frustum", "enable", "disable", "blendFunc", "alphaFunc",
		"depthFunc", "depthMask", "colorMask", "cullFace", "shadeModel", "logicOp", "lineWidth", "polygonOffset",
		"viewport", "pixelStorei", "color4f", "color3f", "normal3f", "normal3b", "fogf", "fogfv", "fogi",
		"lightfv", "lightModelfv", "colorMaterial", "genTextures", "bindTexture", "texParameteri", "texImage2D",
		"texSubImage2D", "deleteTextures", "vertexPointer", "texCoordPointer", "colorPointer", "normalPointer",
		"enableClientState", "drawArrays", "disableClientState", "begin", "texCoord2f", "end", "genBuffersARB",
		"bindBufferARB", "bufferDataARB", "genLists", "newList", "endList", "callList", "callLists",
		"deleteLists", "clearColor", "clearDepth", "clear", "readPixels", "finish", "pushMatrix", "popMatrix",
		"rotatef", "scalef" };

	for (const char *name : expectedOnce)
		ctx.checkEqual(sink.countOf(name), 1, std::string("backend received ") + name + " exactly once");

	ctx.checkEqual(sink.countOf("matrixMode"), 3, "backend received every matrix mode change");
	ctx.checkEqual(sink.countOf("loadIdentity"), 3, "backend received every glLoadIdentity");
	ctx.checkEqual(sink.countOf("translatef"), 2, "backend received the translate outside and inside the list");
	ctx.checkEqual(sink.countOf("vertex3f"), 3, "backend received every immediate vertex");

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_dispatch, rejected_calls_are_not_forwarded)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	// A call that fails validation must not reach the backend at all: GL raises
	// the error and changes nothing.
	glEnable(0x0B24);
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA_SATURATE);
	glDepthFunc(GL_TEXTURE_2D);
	glCullFace(GL_LIGHT0);
	glShadeModel(GL_LIGHT0);
	glLineWidth(0.0f);
	glViewport(0, 0, -1, -1);
	glPixelStorei(GL_PACK_ALIGNMENT, 3);
	glMatrixMode(GL_LIGHT0);
	glBindTexture(0x0DE0, 1);
	glFogi(GL_FOG_MODE, GL_NEAREST);
	glDrawArrays(0x4321, 0, 3);

	ctx.checkEqual(static_cast<long long>(sink.calls.size()), 0, "no rejected call reached the backend");
	ctx.checkEqual(legacygl::context().pendingError(), GL_INVALID_ENUM, "the first error was latched");

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_dispatch, list_compilation_still_forwards_the_call_stream)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	// The native backend builds its own display list, so the calls between
	// glNewList and glEndList have to reach it even though the semantic core is
	// only recording them.
	const GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glColor3f(1.0f, 0.0f, 0.0f);
	glTranslatef(1.0f, 0.0f, 0.0f);
	glBegin(GL_TRIANGLES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();
	glEndList();

	ctx.checkEqual(sink.countOf("color3f"), 1, "a compiled colour command reached the backend");
	ctx.checkEqual(sink.countOf("translatef"), 1, "a compiled matrix command reached the backend");
	ctx.checkEqual(sink.countOf("vertex3f"), 3, "compiled immediate vertices reached the backend");
	ctx.checkEqual(sink.countOf("newList"), 1, "glNewList reached the backend");
	ctx.checkEqual(sink.countOf("endList"), 1, "glEndList reached the backend");

	// Executing the list forwards only the call itself: the backend replays its
	// own copy, so the core must not re-send the recorded commands.
	glCallList(list);
	ctx.checkEqual(sink.countOf("callList"), 1, "glCallList reached the backend");
	ctx.checkEqual(sink.countOf("translatef"), 1, "the replay did not duplicate the compiled commands");
	ctx.checkEqual(sink.countOf("vertex3f"), 3, "the replay did not duplicate the compiled vertices");

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_resolved, array_draw_uses_current_not_previous_geometry)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	const GLfloat first[] = {
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f,
		7.0f, 8.0f, 9.0f
	};
	const GLfloat second[] = {
		-1.0f, -2.0f, -3.0f,
		-4.0f, -5.0f, -6.0f,
		-7.0f, -8.0f, -9.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, first);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glVertexPointer(3, GL_FLOAT, 0, second);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	if (ctx.check(sink.resolvedGeometry.size() == 2, "both array draws emitted resolved geometry"))
	{
		ctx.checkEqualBits(sink.resolvedGeometry[0].vertices[0].x, 1.0f,
			"the first callback sees the first draw, not empty prior geometry");
		ctx.checkEqualBits(sink.resolvedGeometry[1].vertices[0].x, -1.0f,
			"the second callback sees the current draw, not the previous draw");
		ctx.checkEqualBits(sink.resolvedGeometry[1].vertices[2].z, -9.0f,
			"the current draw is complete before the callback");
	}
	ctx.checkEqual(static_cast<long long>(sink.resolvedPrimitives.size()), 2,
		"each array draw emitted one canonical primitive batch");
	if (sink.resolvedPrimitives.size() == 2)
	{
		ctx.checkEqual(static_cast<long long>(sink.resolvedPrimitives[1].primitives.size()), 1,
			"the canonical batch contains the current triangle");
	}

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_resolved, immediate_draw_carries_resolved_state)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(1.0f, 2.0f, 3.0f);
	glScalef(2.0f, 2.0f, 2.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(3.0f, 4.0f, 1.0f);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.25f, 0.5f, 0.0f);
	glMatrixMode(GL_MODELVIEW);

	const GLenum enables[] = {
		GL_TEXTURE_2D, GL_DEPTH_TEST, GL_ALPHA_TEST, GL_BLEND,
		GL_CULL_FACE, GL_FOG, GL_LIGHTING, GL_COLOR_MATERIAL,
		GL_RESCALE_NORMAL, GL_NORMALIZE, GL_COLOR_LOGIC_OP,
		GL_POLYGON_OFFSET_FILL, GL_SCISSOR_TEST, GL_STENCIL_TEST,
		GL_LINE_SMOOTH, GL_LIGHT0
	};
	for (GLenum cap : enables)
		glEnable(cap);
	glDisable(GL_DITHER);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GEQUAL, 0.25f);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
	glCullFace(GL_FRONT);
	glShadeModel(GL_FLAT);
	glLogicOp(GL_XOR);
	glLineWidth(2.5f);
	glPolygonOffset(1.5f, -2.0f);
	glViewport(4, 5, 640, 480);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.75f);
	glFogf(GL_FOG_START, 2.0f);
	glFogf(GL_FOG_END, 12.0f);
	const GLfloat fogColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);

	const GLfloat lightDiffuse[] = { 0.6f, 0.5f, 0.4f, 1.0f };
	const GLfloat lightAmbient[] = { 0.05f, 0.1f, 0.15f, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbient);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	const unsigned char texturePixels[16] = {};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturePixels);

	glColor4f(0.2f, 0.3f, 0.4f, 0.5f);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.75f, 0.25f);
	glBegin(GL_TRIANGLES);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glEnd();

	if (ctx.check(sink.resolvedDraws.size() == 1, "the immediate draw emitted once"))
	{
		const legacygl::ResolvedDraw &draw = sink.resolvedDraws[0];
		ctx.checkEqualBits(draw.modelView.m[0], 2.0f, "model-view matrix is snapshotted");
		ctx.checkEqualBits(draw.modelView.m[12], 1.0f, "model-view translation is snapshotted");
		ctx.checkEqualBits(draw.projection.m[0], 3.0f, "projection matrix is snapshotted");
		ctx.checkEqualBits(draw.textureMatrix.m[12], 0.25f, "texture matrix is snapshotted");
		ctx.checkEqualBits(draw.normal.m[0], 0.5f, "normal matrix is derived at draw time");
		ctx.checkEqualBits(draw.normalRescaleFactor, 2.0f, "normal rescale factor is derived separately");

		ctx.check(draw.enables.texture2D && draw.enables.depthTest && draw.enables.alphaTest &&
			draw.enables.blend && draw.enables.cullFace && draw.enables.fog && draw.enables.lighting &&
			draw.enables.colorMaterial && draw.enables.rescaleNormal && draw.enables.normalize &&
			draw.enables.colorLogicOp && draw.enables.polygonOffsetFill && draw.enables.scissorTest &&
			draw.enables.stencilTest && draw.enables.lineSmooth && !draw.enables.dither,
			"all exercised enables are snapshotted");
		ctx.checkEqual(draw.pipeline.blendSource, GL_SRC_ALPHA, "blend source is snapshotted");
		ctx.checkEqual(draw.pipeline.blendDestination, GL_ONE_MINUS_SRC_ALPHA,
			"blend destination is snapshotted");
		ctx.checkEqual(draw.pipeline.alphaFunction, GL_GEQUAL, "alpha function is snapshotted");
		ctx.checkEqualBits(draw.pipeline.alphaReference, 0.25f, "alpha reference is snapshotted");
		ctx.checkEqual(draw.pipeline.depthFunction, GL_LEQUAL, "depth function is snapshotted");
		ctx.check(!draw.pipeline.depthWrite, "depth write mask is snapshotted");
		ctx.check(draw.pipeline.colorWrite[0] && !draw.pipeline.colorWrite[1] &&
			draw.pipeline.colorWrite[2] && !draw.pipeline.colorWrite[3], "colour write mask is snapshotted");
		ctx.checkEqual(draw.pipeline.cullFaceMode, GL_FRONT, "cull face is snapshotted");
		ctx.checkEqual(draw.pipeline.frontFaceMode, GL_CCW, "front-face winding is snapshotted");
		ctx.checkEqual(draw.pipeline.shadeModel, GL_FLAT, "shade model is snapshotted");
		ctx.checkEqual(draw.pipeline.logicOpcode, GL_XOR, "logic operation is snapshotted");
		ctx.checkEqualBits(draw.pipeline.lineWidth, 2.5f, "line width is snapshotted");
		ctx.checkEqualBits(draw.pipeline.polygonOffsetFactor, 1.5f, "polygon offset factor is snapshotted");
		ctx.checkEqualBits(draw.pipeline.polygonOffsetUnits, -2.0f, "polygon offset units are snapshotted");
		ctx.checkEqual(draw.pipeline.viewport[0], 4, "viewport x is snapshotted");
		ctx.checkEqual(draw.pipeline.viewport[2], 640, "viewport width is snapshotted");

		ctx.checkEqual(draw.fog.mode, GL_LINEAR, "fog mode is snapshotted");
		ctx.checkEqualBits(draw.fog.density, 0.75f, "fog density is snapshotted");
		ctx.checkEqualBits(draw.fog.start, 2.0f, "fog start is snapshotted");
		ctx.checkEqualBits(draw.fog.end, 12.0f, "fog end is snapshotted");
		ctx.checkEqualBits(draw.fog.color[2], 0.3f, "fog colour is snapshotted");
		ctx.checkEqual(draw.fog.distanceMode, GL_EYE_RADIAL_NV, "radial fog mode is snapshotted");

		ctx.check(draw.lighting.lights[0].enabled, "light enable is snapshotted");
		ctx.checkEqualBits(draw.lighting.lights[0].diffuse[0], 0.6f, "light state is snapshotted");
		ctx.checkEqualBits(draw.lighting.modelAmbient[2], 0.15f, "light model is snapshotted");
		ctx.checkEqual(draw.lighting.colorMaterialFace, GL_FRONT, "colour-material face is snapshotted");
		ctx.checkEqual(draw.lighting.colorMaterialMode, GL_DIFFUSE, "colour-material mode is snapshotted");
		ctx.checkEqualBits(draw.lighting.frontMaterial.diffuse[0], 0.2f,
			"colour material's updated front material is snapshotted");
		ctx.checkEqualBits(draw.lighting.backMaterial.diffuse[0], 0.8f,
			"the unaffected back material is snapshotted separately");

		ctx.checkEqual(draw.texture.name, texture, "logical texture binding is snapshotted");
		ctx.checkEqual(draw.texture.minFilter, GL_LINEAR, "texture minification is snapshotted");
		ctx.checkEqual(draw.texture.magFilter, GL_NEAREST, "texture magnification is snapshotted");
		ctx.checkEqual(draw.texture.wrapS, GL_CLAMP, "legacy clamp is preserved in the snapshot");
		ctx.checkEqual(draw.texture.wrapT, GL_CLAMP_TO_EDGE, "edge clamp is preserved separately");
		ctx.checkEqual(draw.texture.level0Width, 2, "level-zero width is snapshotted");
		ctx.checkEqual(draw.texture.level0Height, 2, "level-zero height is snapshotted");
		ctx.checkEqual(draw.texture.level0InternalFormat, GL_RGBA, "level-zero internal format is snapshotted");
		ctx.check(draw.texture.level0Defined && draw.texture.complete, "texture completeness is snapshotted");
	}

	if (ctx.check(sink.resolvedGeometry.size() == 1, "the immediate geometry was delivered"))
	{
		ctx.checkEqualBits(sink.resolvedGeometry[0].vertices[0].r, 0.2f,
			"immediate primary colour is canonical geometry");
		ctx.checkEqualBits(sink.resolvedGeometry[0].vertices[0].t, 0.25f,
			"immediate texture coordinate is canonical geometry");
		ctx.checkEqualBits(sink.resolvedGeometry[0].vertices[0].ny, 1.0f,
			"immediate normal is canonical geometry");
	}

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_resolved, compile_only_list_defers_state_clear_and_draw)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	const GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glTranslatef(5.0f, 0.0f, 0.0f);
	glColor4f(0.25f, 0.5f, 0.75f, 1.0f);
	glEnable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_DITHER);
	glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glClearColor(0.1f, 0.2f, 0.3f, 0.4f);
	glClearDepth(0.625);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();
	glEndList();

	ctx.checkEqual(static_cast<long long>(sink.resolvedCalls.size()), 0,
		"GL_COMPILE emits no resolved work");

	glCallList(list);
	ctx.checkEqual(static_cast<long long>(sink.resolvedCalls.size()), 2,
		"list replay emits one clear and one draw");
	if (sink.resolvedCalls.size() == 2)
	{
		ctx.checkEqual(sink.resolvedCalls[0], "clear", "the resolved clear keeps list order");
		ctx.checkEqual(sink.resolvedCalls[1], "draw", "the resolved draw keeps list order");
	}

	if (ctx.check(sink.resolvedClears.size() == 1, "the list clear emitted exactly once"))
	{
		const legacygl::ResolvedClear &clear = sink.resolvedClears[0];
		ctx.checkEqual(clear.sequence, legacygl::context().sequence(),
			"list clear carries the execution call sequence");
		ctx.checkEqual(clear.mask, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
			"clear mask is captured at execution");
		ctx.checkEqualBits(clear.color[0], 0.1f, "clear colour is resolved from list state");
		ctx.checkEqualBits(clear.color[3], 0.4f, "clear alpha is resolved from list state");
		ctx.checkEqualBits(clear.depth, 0.625, "clear depth is resolved from list state");
		ctx.check(clear.colorWrite[0] && !clear.colorWrite[1] && clear.colorWrite[2] && !clear.colorWrite[3],
			"clear colour write mask is resolved at execution");
		ctx.check(!clear.depthWrite, "clear depth write mask is resolved at execution");
		ctx.check(clear.scissorTest, "clear scissor enable is resolved at execution");
		ctx.check(!clear.dither, "clear dither enable is resolved at execution");
	}

	if (ctx.check(sink.resolvedDraws.size() == 1, "the list draw emitted exactly once"))
	{
		ctx.checkEqualBits(sink.resolvedDraws[0].modelView.m[12], 5.0f,
			"list matrix state is resolved before the draw");
		ctx.check(sink.resolvedDraws[0].enables.blend, "list enable state is resolved before the draw");
	}
	if (ctx.check(sink.resolvedGeometry.size() == 1, "the replay delivered list geometry"))
	{
		ctx.checkEqualBits(sink.resolvedGeometry[0].vertices[0].r, 0.25f,
			"list current colour is resolved into replayed geometry");
	}

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_resolved, texture_upload_carries_identity_alignment_and_pixels)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	const unsigned char image[32] = {};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	const unsigned char patch[6] = {};
	glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 2, 1, GL_RGB, GL_UNSIGNED_BYTE, patch);

	if (ctx.check(sink.resolvedUploads.size() == 2, "full and sub-image uploads emitted"))
	{
		const legacygl::ResolvedTextureUpload &full = sink.resolvedUploads[0];
		ctx.checkEqual(full.texture, texture, "full upload carries the logical texture name");
		ctx.check(!full.subImage, "full upload is classified separately");
		ctx.checkEqual(full.level, 0, "full upload carries its level");
		ctx.checkEqual(full.width, 3, "full upload carries its width");
		ctx.checkEqual(full.height, 2, "full upload carries its height");
		ctx.checkEqual(full.internalFormat, GL_RGBA, "full upload carries its internal format");
		ctx.checkEqual(full.sourceFormat, GL_RGB, "full upload carries its source format");
		ctx.checkEqual(full.sourceType, GL_UNSIGNED_BYTE, "full upload carries its source type");
		ctx.checkEqual(full.unpackAlignment, 8, "full upload snapshots unpack alignment");
		ctx.check(full.pixels == image, "full upload preserves the ephemeral pixel pointer");

		const legacygl::ResolvedTextureUpload &sub = sink.resolvedUploads[1];
		ctx.checkEqual(sub.texture, texture, "sub-image carries the logical texture name");
		ctx.check(sub.subImage, "sub-image upload is classified separately");
		ctx.checkEqual(sub.x, 1, "sub-image carries x offset");
		ctx.checkEqual(sub.y, 1, "sub-image carries y offset");
		ctx.checkEqual(sub.width, 2, "sub-image carries width");
		ctx.checkEqual(sub.height, 1, "sub-image carries height");
		ctx.checkEqual(sub.internalFormat, GL_RGBA, "sub-image carries the level internal format");
		ctx.checkEqual(sub.unpackAlignment, 1, "sub-image snapshots current unpack alignment");
		ctx.check(sub.pixels == patch, "sub-image preserves the ephemeral pixel pointer");
	}

	const std::size_t beforeRejected = sink.resolvedUploads.size();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 3, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, patch);
	ctx.checkEqual(static_cast<long long>(sink.resolvedUploads.size()), static_cast<long long>(beforeRejected),
		"a rejected upload emits no resolved work");
	glGetError();

	const GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, patch);
	glEndList();
	ctx.checkEqual(static_cast<long long>(sink.resolvedUploads.size()), static_cast<long long>(beforeRejected),
		"a compile-only upload emits no resolved work");

	legacygl::setSink(legacygl::nullSink());
}

HEADLESS_TEST(legacygl_resolved, readback_carries_pack_state_and_destination)
{
	legacyglTest::begin();

	RecordingSink sink;
	legacygl::setSink(&sink);

	glPixelStorei(GL_PACK_ALIGNMENT, 8);
	unsigned char destination[128] = {};
	glReadPixels(2, 3, 4, 5, GL_BGRA_EXT, GL_UNSIGNED_BYTE, destination);

	if (ctx.check(sink.resolvedReadbacks.size() == 1, "valid readback emitted once"))
	{
		const legacygl::ResolvedReadback &readback = sink.resolvedReadbacks[0];
		ctx.checkEqual(readback.x, 2, "readback carries x");
		ctx.checkEqual(readback.y, 3, "readback carries y");
		ctx.checkEqual(readback.width, 4, "readback carries width");
		ctx.checkEqual(readback.height, 5, "readback carries height");
		ctx.checkEqual(readback.format, GL_BGRA_EXT, "readback carries format");
		ctx.checkEqual(readback.type, GL_UNSIGNED_BYTE, "readback carries type");
		ctx.checkEqual(readback.packAlignment, 8, "readback snapshots pack alignment");
		ctx.check(readback.pixels == destination, "readback preserves the destination pointer");
	}

	glReadPixels(0, 0, -1, 1, GL_RGBA, GL_UNSIGNED_BYTE, destination);
	ctx.checkEqual(static_cast<long long>(sink.resolvedReadbacks.size()), 1,
		"a rejected readback emits no resolved work");

	legacygl::setSink(legacygl::nullSink());
}
