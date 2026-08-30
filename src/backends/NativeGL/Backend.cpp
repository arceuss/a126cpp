// Native compatibility-OpenGL backend: the behavioural oracle.
//
// This translation unit is the only place in the renderer that talks to a GL
// loader. It forwards each frontend call to the corresponding compatibility
// entry point in the same order the game issued it, so the pixels this backend
// produces are the reference every translated backend is compared against.
//
// legacygl/LegacyGL.h must not be included here: glad redefines gl* as macros
// pointing at its own function pointers. Sink.h deliberately uses plain scalar
// types so this file needs neither header from the other side of the boundary.

#include <glad/glad.h>

#include "backends/Backend.h"
#include "backends/OpenGL/Context.h"
#include "legacygl/Sink.h"

namespace legacygl
{

class NativeGLSink : public Sink
{
public:
	void matrixMode(unsigned int mode) override { glMatrixMode(mode); }
	void loadIdentity() override { glLoadIdentity(); }
	void pushMatrix() override { glPushMatrix(); }
	void popMatrix() override { glPopMatrix(); }
	void translatef(float x, float y, float z) override { glTranslatef(x, y, z); }
	void rotatef(float angle, float x, float y, float z) override { glRotatef(angle, x, y, z); }
	void scalef(float x, float y, float z) override { glScalef(x, y, z); }
	void scaled(double x, double y, double z) override { glScaled(x, y, z); }
	void ortho(double left, double right, double bottom, double top, double zNear, double zFar) override
	{
		glOrtho(left, right, bottom, top, zNear, zFar);
	}
	void frustum(double left, double right, double bottom, double top, double zNear, double zFar) override
	{
		glFrustum(left, right, bottom, top, zNear, zFar);
	}

	void enable(unsigned int cap) override { glEnable(cap); }
	void disable(unsigned int cap) override { glDisable(cap); }
	void blendFunc(unsigned int sfactor, unsigned int dfactor) override { glBlendFunc(sfactor, dfactor); }
	void alphaFunc(unsigned int func, float ref) override { glAlphaFunc(func, ref); }
	void depthFunc(unsigned int func) override { glDepthFunc(func); }
	void depthMask(unsigned char flag) override { glDepthMask(flag); }
	void colorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) override
	{
		glColorMask(red, green, blue, alpha);
	}
	void cullFace(unsigned int mode) override { glCullFace(mode); }
	void shadeModel(unsigned int mode) override { glShadeModel(mode); }
	void logicOp(unsigned int opcode) override { glLogicOp(opcode); }
	void lineWidth(float width) override { glLineWidth(width); }
	void polygonOffset(float factor, float units) override { glPolygonOffset(factor, units); }
	void viewport(int x, int y, int width, int height) override { glViewport(x, y, width, height); }
	void pixelStorei(unsigned int pname, int param) override { glPixelStorei(pname, param); }

	void color4f(float red, float green, float blue, float alpha) override { glColor4f(red, green, blue, alpha); }
	void color3f(float red, float green, float blue) override { glColor3f(red, green, blue); }
	void normal3f(float nx, float ny, float nz) override { glNormal3f(nx, ny, nz); }
	void normal3b(signed char nx, signed char ny, signed char nz) override { glNormal3b(nx, ny, nz); }

	void fogf(unsigned int pname, float param) override { glFogf(pname, param); }
	void fogfv(unsigned int pname, const float *params) override { glFogfv(pname, params); }
	void fogi(unsigned int pname, int param) override { glFogi(pname, param); }

	void lightfv(unsigned int light, unsigned int pname, const float *params) override
	{
		glLightfv(light, pname, params);
	}
	void lightModelfv(unsigned int pname, const float *params) override { glLightModelfv(pname, params); }
	void colorMaterial(unsigned int face, unsigned int mode) override { glColorMaterial(face, mode); }

	void genTextures(int n, unsigned int *textures) override { glGenTextures(n, textures); }
	void deleteTextures(int n, const unsigned int *textures) override { glDeleteTextures(n, textures); }
	void bindTexture(unsigned int target, unsigned int texture) override { glBindTexture(target, texture); }
	void texParameteri(unsigned int target, unsigned int pname, int param) override
	{
		glTexParameteri(target, pname, param);
	}
	void texImage2D(unsigned int target, int level, int internalformat, int width, int height, int border,
		unsigned int format, unsigned int type, const void *pixels) override
	{
		glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
	}
	void texSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height,
		unsigned int format, unsigned int type, const void *pixels) override
	{
		glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
	}

	void enableClientState(unsigned int array) override { glEnableClientState(array); }
	void disableClientState(unsigned int array) override { glDisableClientState(array); }
	void vertexPointer(int size, unsigned int type, int stride, const void *pointer) override
	{
		glVertexPointer(size, type, stride, pointer);
	}
	void texCoordPointer(int size, unsigned int type, int stride, const void *pointer) override
	{
		glTexCoordPointer(size, type, stride, pointer);
	}
	void colorPointer(int size, unsigned int type, int stride, const void *pointer) override
	{
		glColorPointer(size, type, stride, pointer);
	}
	void normalPointer(unsigned int type, int stride, const void *pointer) override
	{
		glNormalPointer(type, stride, pointer);
	}
	void drawArrays(unsigned int mode, int first, int count) override { glDrawArrays(mode, first, count); }

	void begin(unsigned int mode) override { glBegin(mode); }
	void end() override { glEnd(); }
	void vertex3f(float x, float y, float z) override { glVertex3f(x, y, z); }
	void texCoord2f(float s, float t) override { glTexCoord2f(s, t); }

	void genBuffersARB(int n, unsigned int *buffers) override { glGenBuffersARB(n, buffers); }
	void bindBufferARB(unsigned int target, unsigned int buffer) override { glBindBufferARB(target, buffer); }
	void bufferDataARB(unsigned int target, std::ptrdiff_t size, const void *data, unsigned int usage) override
	{
		glBufferDataARB(target, static_cast<GLsizeiptrARB>(size), data, usage);
	}

	unsigned int genLists(int range) override { return glGenLists(range); }
	void newList(unsigned int list, unsigned int mode) override { glNewList(list, mode); }
	void endList() override { glEndList(); }
	void callList(unsigned int list) override { glCallList(list); }
	void callLists(int n, unsigned int type, const void *lists) override { glCallLists(n, type, lists); }
	void deleteLists(unsigned int list, int range) override { glDeleteLists(list, range); }

	void clear(unsigned int mask) override { glClear(mask); }
	void clearColor(float red, float green, float blue, float alpha) override
	{
		glClearColor(red, green, blue, alpha);
	}
	void clearDepth(double depth) override { glClearDepth(depth); }
	void readPixels(int x, int y, int width, int height, unsigned int format, unsigned int type,
		void *pixels) override
	{
		glReadPixels(x, y, width, height, format, type, pixels);
	}
	void finish() override { glFinish(); }

	// Validation support. These read the driver's state so the semantic core's
	// answers can be diffed against the oracle; they are never used to answer a
	// frontend query.
	bool queryFloatv(unsigned int pname, float *params) override
	{
		if (glGetFloatv == nullptr)
			return false;
		glGetFloatv(pname, params);
		return true;
	}

	bool queryError(unsigned int *error) override
	{
		if (glGetError == nullptr)
			return false;
		*error = glGetError();
		return true;
	}
};

static NativeGLSink theNativeSink;

}

namespace renderbackend
{

const Configuration &configuration()
{
	static const Configuration config = {
		"native",
		1, 1, OpenGLProfile::Compatibility,
		0, 0, OpenGLProfile::None,
		false,
		true
	};
	return config;
}

void initialize()
{
	openglbackend::initialize(configuration());
}

void present()
{
	openglbackend::present();
}

void shutdown()
{
	openglbackend::shutdown();
}

bool hasCapability(const char *capability)
{
	return openglbackend::hasCapability(capability);
}

legacygl::Sink *sink()
{
	return &legacygl::theNativeSink;
}

}
