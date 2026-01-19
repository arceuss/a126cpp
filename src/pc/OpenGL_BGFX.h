#pragma once

#ifdef USE_BGFX

// When USE_BGFX is enabled, redirect OpenGL calls to bgfx compatibility layer

#include "lwjgl/BGFXCompat.h"
#include <glad/glad.h> // For GL constants

// Redirect OpenGL functions to bgfx compatibility layer
#define glGenLists(n) (lwjgl::BGFXCompat::genLists(n))
#define glNewList(list, mode) lwjgl::BGFXCompat::newList(list, mode)
#define glEndList() lwjgl::BGFXCompat::endList()
#define glCallList(list) lwjgl::BGFXCompat::callList(list)
#define glCallLists(n, type, lists) lwjgl::BGFXCompat::callLists(n, type, lists)
#define glDeleteLists(list, range) /* bgfx handles cleanup automatically */

// Matrix functions
#define glPushMatrix() lwjgl::BGFXCompat::pushMatrix()
#define glPopMatrix() lwjgl::BGFXCompat::popMatrix()
#define glLoadIdentity() lwjgl::BGFXCompat::loadIdentity()
#define glTranslatef(x, y, z) lwjgl::BGFXCompat::translatef(x, y, z)
#define glScalef(x, y, z) lwjgl::BGFXCompat::scalef(x, y, z)
#define glRotatef(angle, x, y, z) lwjgl::BGFXCompat::rotatef(angle, x, y, z)
#define glMultMatrixf(m) lwjgl::BGFXCompat::multMatrixf(m)
#define glLoadMatrixf(m) lwjgl::BGFXCompat::loadMatrixf(m)
#define glMatrixMode(mode) lwjgl::BGFXCompat::matrixMode(mode)

// State functions
#define glEnable(cap) lwjgl::BGFXCompat::enable(cap)
#define glDisable(cap) lwjgl::BGFXCompat::disable(cap)
#define glBlendFunc(sfactor, dfactor) lwjgl::BGFXCompat::blendFunc(sfactor, dfactor)
#define glAlphaFunc(func, ref) lwjgl::BGFXCompat::alphaFunc(func, ref)
#define glDepthFunc(func) lwjgl::BGFXCompat::depthFunc(func)
#define glCullFace(mode) lwjgl::BGFXCompat::cullFace(mode)

// Viewport and clear
#define glViewport(x, y, width, height) lwjgl::BGFXCompat::viewport(x, y, width, height)
#define glClear(mask) lwjgl::BGFXCompat::clear(mask)
#define glClearColor(red, green, blue, alpha) lwjgl::BGFXCompat::clearColor(red, green, blue, alpha)
#define glClearDepth(depth) lwjgl::BGFXCompat::clearDepth(depth)

// Vertex array functions (captured during display list compilation)
#define glVertexPointer(size, type, stride, pointer) lwjgl::BGFXCompat::vertexPointer(size, type, stride, pointer)
#define glTexCoordPointer(size, type, stride, pointer) lwjgl::BGFXCompat::texCoordPointer(size, type, stride, pointer)
#define glColorPointer(size, type, stride, pointer) lwjgl::BGFXCompat::colorPointer(size, type, stride, pointer)
#define glNormalPointer(type, stride, pointer) lwjgl::BGFXCompat::normalPointer(type, stride, pointer)
#define glEnableClientState(cap) lwjgl::BGFXCompat::enableClientState(cap)
#define glDisableClientState(cap) lwjgl::BGFXCompat::disableClientState(cap)
#define glDrawArrays(mode, first, count) lwjgl::BGFXCompat::drawArrays(mode, first, count)

// Texture functions (need texture ID mapping)
#define glBindTexture(target, texture) lwjgl::BGFXCompat::bindTexture(target, texture)
#define glTexParameteri(target, pname, param) lwjgl::BGFXCompat::texParameteri(target, pname, param)

// Fog
#define glFogf(pname, param) lwjgl::BGFXCompat::fogf(pname, param)
#define glFogfv(pname, params) lwjgl::BGFXCompat::fogfv(pname, params)
#define glFogi(pname, param) lwjgl::BGFXCompat::fogi(pname, param)

// Shade model
#define glShadeModel(mode) lwjgl::BGFXCompat::shadeModel(mode)

// Color mask
#define glColorMask(r, g, b, a) /* TODO: map to bgfx color write mask */

// Additional functions that might be needed but aren't redirected yet
// These will fall back to real OpenGL (which won't exist) or need to be stubbed
#define glBegin(mode) /* Not used - Tesselator handles this */
#define glEnd() /* Not used - Tesselator handles this */
#define glVertex3f(x, y, z) /* Not used - Tesselator handles this */
#define glTexCoord2f(u, v) /* Not used - Tesselator handles this */
#define glColor3f(r, g, b) /* Not used - Tesselator handles this */
#define glColor4f(r, g, b, a) /* Not used - Tesselator handles this */
#define glNormal3f(x, y, z) /* Not used - Tesselator handles this */

// Functions that might need special handling
#define glGetString(name) nullptr // Return nullptr for bgfx
#define glGetStringi(name, index) nullptr // Return nullptr for bgfx

#else

// When not using bgfx, use real OpenGL
#include <glad/glad.h>

#endif // USE_BGFX
