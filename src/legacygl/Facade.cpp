#include "legacygl/Context.h"
#include "legacygl/LegacyGL.h"

// The frontend entry points. Every gl* call the game issues arrives here and
// goes straight to the semantic core, which owns state and forwards the call to
// the active backend. Nothing is filtered, reordered or coalesced on the way:
// the call stream a backend sees is the call stream the game wrote.

void glMatrixMode(GLenum mode)
{
	legacygl::context().matrixMode(mode);
}

void glLoadIdentity()
{
	legacygl::context().loadIdentity();
}

void glPushMatrix()
{
	legacygl::context().pushMatrix();
}

void glPopMatrix()
{
	legacygl::context().popMatrix();
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	legacygl::context().translatef(x, y, z);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	legacygl::context().rotatef(angle, x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	legacygl::context().scalef(x, y, z);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
	legacygl::context().scaled(x, y, z);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	legacygl::context().ortho(left, right, bottom, top, zNear, zFar);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	legacygl::context().frustum(left, right, bottom, top, zNear, zFar);
}

void glEnable(GLenum cap)
{
	legacygl::context().enable(cap);
}

void glDisable(GLenum cap)
{
	legacygl::context().disable(cap);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	legacygl::context().blendFunc(sfactor, dfactor);
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
	legacygl::context().alphaFunc(func, ref);
}

void glDepthFunc(GLenum func)
{
	legacygl::context().depthFunc(func);
}

void glDepthMask(GLboolean flag)
{
	legacygl::context().depthMask(flag);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	legacygl::context().colorMask(red, green, blue, alpha);
}

void glCullFace(GLenum mode)
{
	legacygl::context().cullFace(mode);
}

void glShadeModel(GLenum mode)
{
	legacygl::context().shadeModel(mode);
}

void glLogicOp(GLenum opcode)
{
	legacygl::context().logicOp(opcode);
}

void glLineWidth(GLfloat width)
{
	legacygl::context().lineWidth(width);
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
	legacygl::context().polygonOffset(factor, units);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	legacygl::context().viewport(x, y, width, height);
}

void glPixelStorei(GLenum pname, GLint param)
{
	legacygl::context().pixelStorei(pname, param);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	legacygl::context().color3f(red, green, blue);
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	legacygl::context().color4f(red, green, blue, alpha);
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	legacygl::context().normal3f(nx, ny, nz);
}

void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
	legacygl::context().normal3b(nx, ny, nz);
}

void glFogf(GLenum pname, GLfloat param)
{
	legacygl::context().fogf(pname, param);
}

void glFogfv(GLenum pname, const GLfloat *params)
{
	legacygl::context().fogfv(pname, params);
}

void glFogi(GLenum pname, GLint param)
{
	legacygl::context().fogi(pname, param);
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	legacygl::context().lightfv(light, pname, params);
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
	legacygl::context().lightModelfv(pname, params);
}

void glColorMaterial(GLenum face, GLenum mode)
{
	legacygl::context().colorMaterial(face, mode);
}

void glGenTextures(GLsizei n, GLuint *textures)
{
	legacygl::context().genTextures(n, textures);
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
	legacygl::context().deleteTextures(n, textures);
}

void glBindTexture(GLenum target, GLuint texture)
{
	legacygl::context().bindTexture(target, texture);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	legacygl::context().texParameteri(target, pname, param);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
	GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	legacygl::context().texImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const GLvoid *pixels)
{
	legacygl::context().texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glEnableClientState(GLenum array)
{
	legacygl::context().enableClientState(array);
}

void glDisableClientState(GLenum array)
{
	legacygl::context().disableClientState(array);
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	legacygl::context().vertexPointer(size, type, stride, pointer);
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	legacygl::context().texCoordPointer(size, type, stride, pointer);
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	legacygl::context().colorPointer(size, type, stride, pointer);
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	legacygl::context().normalPointer(type, stride, pointer);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	legacygl::context().drawArrays(mode, first, count);
}

void glBegin(GLenum mode)
{
	legacygl::context().begin(mode);
}

void glEnd()
{
	legacygl::context().end();
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	legacygl::context().vertex3f(x, y, z);
}

void glTexCoord2f(GLfloat s, GLfloat t)
{
	legacygl::context().texCoord2f(s, t);
}

void glGenBuffersARB(GLsizei n, GLuint *buffers)
{
	legacygl::context().genBuffersARB(n, buffers);
}

void glBindBufferARB(GLenum target, GLuint buffer)
{
	legacygl::context().bindBufferARB(target, buffer);
}

void glBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage)
{
	legacygl::context().bufferDataARB(target, size, data, usage);
}

GLuint glGenLists(GLsizei range)
{
	return legacygl::context().genLists(range);
}

void glNewList(GLuint list, GLenum mode)
{
	legacygl::context().newList(list, mode);
}

void glEndList()
{
	legacygl::context().endList();
}

void glCallList(GLuint list)
{
	legacygl::context().callList(list);
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
	legacygl::context().callLists(n, type, lists);
}

void glDeleteLists(GLuint list, GLsizei range)
{
	legacygl::context().deleteLists(list, range);
}

void glClear(GLbitfield mask)
{
	legacygl::context().clear(mask);
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	legacygl::context().clearColor(red, green, blue, alpha);
}

void glClearDepth(GLclampd depth)
{
	legacygl::context().clearDepth(depth);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	legacygl::context().readPixels(x, y, width, height, format, type, pixels);
}

void glFinish()
{
	legacygl::context().finish();
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
	legacygl::context().getFloatv(pname, params);
}

GLenum glGetError()
{
	return legacygl::context().getError();
}
