// OpenGL 2.1 rendering backend implementation
// This is the ONLY file that includes <glad/glad.h> for rendering calls.
// All gl*() functions are implemented here by calling the GLAD function pointers.

#include <glad/glad.h>

#define _USE_MATH_DEFINES
#include <cmath>

// ============================================================================
// Undef all GLAD convenience macros so we can define our own functions
// ============================================================================

// Matrix
#undef glMatrixMode
#undef glLoadIdentity
#undef glPushMatrix
#undef glPopMatrix
#undef glTranslatef
#undef glRotatef
#undef glScalef
#undef glScaled
#undef glMultMatrixf
#undef glOrtho
#undef glFrustum

// State
#undef glEnable
#undef glDisable

// Blending
#undef glBlendFunc

// Color
#undef glColor3f
#undef glColor4f

// Depth
#undef glDepthMask
#undef glDepthFunc
#undef glClearDepth

// Alpha
#undef glAlphaFunc

// Fog
#undef glFogf
#undef glFogfv
#undef glFogi

// Lighting
#undef glLightfv
#undef glLightModelfv
#undef glColorMaterial
#undef glNormal3f
#undef glShadeModel

// Clear / Viewport
#undef glClear
#undef glClearColor
#undef glViewport

// Textures
#undef glGenTextures
#undef glDeleteTextures
#undef glBindTexture
#undef glTexParameteri
#undef glTexImage2D
#undef glTexSubImage2D
#undef glPixelStorei

// Display Lists
#undef glGenLists
#undef glDeleteLists
#undef glNewList
#undef glEndList
#undef glCallList
#undef glCallLists

// Vertex Arrays
#undef glVertexPointer
#undef glTexCoordPointer
#undef glColorPointer
#undef glNormalPointer
#undef glEnableClientState
#undef glDisableClientState
#undef glDrawArrays

// VBO (GL 1.5)
#undef glGenBuffers
#undef glDeleteBuffers
#undef glBindBuffer
#undef glBufferData

// VBO (ARB)
#undef glGenBuffersARB
#undef glBindBufferARB
#undef glBufferDataARB

// Misc
#undef glColorMask
#undef glCullFace
#undef glLineWidth
#undef glPolygonOffset
#undef glLogicOp

// Query / Debug
#undef glGetFloatv
#undef glGetError
#undef glGetString
#undef glReadPixels
#undef glDebugMessageCallback

// ============================================================================
// Function Implementations - call through GLAD function pointers
// ============================================================================

// --- Matrix Stack ---
void glMatrixMode(GLenum mode)                          { glad_glMatrixMode(mode); }
void glLoadIdentity()                                    { glad_glLoadIdentity(); }
void glPushMatrix()                                      { glad_glPushMatrix(); }
void glPopMatrix()                                       { glad_glPopMatrix(); }
void glTranslatef(GLfloat x, GLfloat y, GLfloat z)      { glad_glTranslatef(x, y, z); }
void glRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) { glad_glRotatef(a, x, y, z); }
void glScalef(GLfloat x, GLfloat y, GLfloat z)           { glad_glScalef(x, y, z); }
void glScaled(GLdouble x, GLdouble y, GLdouble z)        { glad_glScaled(x, y, z); }
void glMultMatrixf(const GLfloat* m)                     { glad_glMultMatrixf(m); }
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
                                                         { glad_glOrtho(l, r, b, t, n, f); }
void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
                                                         { glad_glFrustum(l, r, b, t, n, f); }

// --- State Enable/Disable ---
void glEnable(GLenum cap)  { glad_glEnable(cap); }
void glDisable(GLenum cap) { glad_glDisable(cap); }

// --- Blending ---
void glBlendFunc(GLenum sfactor, GLenum dfactor) { glad_glBlendFunc(sfactor, dfactor); }

// --- Color ---
void glColor3f(GLfloat r, GLfloat g, GLfloat b)             { glad_glColor3f(r, g, b); }
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)  { glad_glColor4f(r, g, b, a); }

// --- Depth ---
void glDepthMask(GLboolean flag)   { glad_glDepthMask(flag); }
void glDepthFunc(GLenum func)      { glad_glDepthFunc(func); }
void glClearDepth(GLdouble depth)  { glad_glClearDepth(depth); }

// --- Alpha ---
void glAlphaFunc(GLenum func, GLclampf ref) { glad_glAlphaFunc(func, ref); }

// --- Fog ---
void glFogf(GLenum pname, GLfloat param)          { glad_glFogf(pname, param); }
void glFogfv(GLenum pname, const GLfloat* params) { glad_glFogfv(pname, params); }
void glFogi(GLenum pname, GLint param)             { glad_glFogi(pname, param); }

// --- Lighting ---
void glLightfv(GLenum light, GLenum pname, const GLfloat* params)   { glad_glLightfv(light, pname, params); }
void glLightModelfv(GLenum pname, const GLfloat* params)             { glad_glLightModelfv(pname, params); }
void glColorMaterial(GLenum face, GLenum mode)                       { glad_glColorMaterial(face, mode); }
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)                  { glad_glNormal3f(nx, ny, nz); }
void glShadeModel(GLenum mode)                                       { glad_glShadeModel(mode); }

// --- Clear / Viewport ---
void glClear(GLbitfield mask)                                   { glad_glClear(mask); }
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) { glad_glClearColor(r, g, b, a); }
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h)        { glad_glViewport(x, y, w, h); }

// --- Textures ---
void glGenTextures(GLsizei n, GLuint* textures)                { glad_glGenTextures(n, textures); }
void glDeleteTextures(GLsizei n, const GLuint* textures)       { glad_glDeleteTextures(n, textures); }
void glBindTexture(GLenum target, GLuint texture)              { glad_glBindTexture(target, texture); }
void glTexParameteri(GLenum target, GLenum pname, GLint param) { glad_glTexParameteri(target, pname, param); }
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const void* pixels)
    { glad_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels); }
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
    { glad_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); }
void glPixelStorei(GLenum pname, GLint param) { glad_glPixelStorei(pname, param); }

// --- Display Lists ---
GLuint glGenLists(GLsizei range)                        { return glad_glGenLists(range); }
void glDeleteLists(GLuint list, GLsizei range)          { glad_glDeleteLists(list, range); }
void glNewList(GLuint list, GLenum mode)                { glad_glNewList(list, mode); }
void glEndList()                                         { glad_glEndList(); }
void glCallList(GLuint list)                             { glad_glCallList(list); }
void glCallLists(GLsizei n, GLenum type, const void* lists) { glad_glCallLists(n, type, lists); }

// --- Vertex Arrays ---
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
    { glad_glVertexPointer(size, type, stride, pointer); }
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
    { glad_glTexCoordPointer(size, type, stride, pointer); }
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
    { glad_glColorPointer(size, type, stride, pointer); }
void glNormalPointer(GLenum type, GLsizei stride, const void* pointer)
    { glad_glNormalPointer(type, stride, pointer); }
void glEnableClientState(GLenum array)  { glad_glEnableClientState(array); }
void glDisableClientState(GLenum array) { glad_glDisableClientState(array); }
void glDrawArrays(GLenum mode, GLint first, GLsizei count) { glad_glDrawArrays(mode, first, count); }

// --- VBO (GL 1.5) ---
void glGenBuffers(GLsizei n, GLuint* buffers)                   { glad_glGenBuffers(n, buffers); }
void glDeleteBuffers(GLsizei n, const GLuint* buffers)           { glad_glDeleteBuffers(n, buffers); }
void glBindBuffer(GLenum target, GLuint buffer)                 { glad_glBindBuffer(target, buffer); }
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
    { glad_glBufferData(target, size, data, usage); }

// --- VBO (ARB extension fallback) ---
void glGenBuffersARB(GLsizei n, GLuint* buffers)                { glad_glGenBuffersARB(n, buffers); }
void glBindBufferARB(GLenum target, GLuint buffer)              { glad_glBindBufferARB(target, buffer); }
void glBufferDataARB(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
    { glad_glBufferDataARB(target, size, data, usage); }

// --- Misc State ---
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { glad_glColorMask(r, g, b, a); }
void glCullFace(GLenum mode)                     { glad_glCullFace(mode); }
void glLineWidth(GLfloat width)                  { glad_glLineWidth(width); }
void glPolygonOffset(GLfloat factor, GLfloat units) { glad_glPolygonOffset(factor, units); }
void glLogicOp(GLenum opcode)                    { glad_glLogicOp(opcode); }

// --- Query / Debug ---
void glGetFloatv(GLenum pname, GLfloat* params)     { glad_glGetFloatv(pname, params); }
GLenum glGetError()                                  { return glad_glGetError(); }
const GLubyte* glGetString(GLenum name)              { return glad_glGetString(name); }
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
    { glad_glReadPixels(x, y, width, height, format, type, pixels); }
void glDebugMessageCallback(GLDEBUGPROC callback, const void* userParam)
    { glad_glDebugMessageCallback(callback, userParam); }

// ============================================================================
// GLU Utility Functions
// ============================================================================

void gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
    double const height = zNear * tanf(fovy * M_PI / 360.0);
    double const width = height * aspect;
    glad_glFrustum(-width, width, -height, height, zNear, zFar);
}

// ============================================================================
// Backend Feature Queries
// ============================================================================

bool RenderBackend_SupportsVBO()
{
    return GLAD_GL_VERSION_1_5 || GLAD_GL_ARB_vertex_buffer_object;
}

bool RenderBackend_SupportsARBVBO()
{
    return GLAD_GL_ARB_vertex_buffer_object;
}
