#pragma once

// Rendering backend interface
// Declares all GL-like functions, types, and constants used by the game code.
// Exactly one Rendering/*.cpp backend is compiled per build (selected by CMake).

#include <cstddef>
#include <cstdint>

// ============================================================================
// GL Type Aliases
// ============================================================================

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// GLAD callback type for debug messages
typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* message, const void* userParam);

// ============================================================================
// GL Constants
// ============================================================================

// Boolean
#define GL_FALSE 0
#define GL_TRUE 1

// Matrix modes
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702

// Matrix query
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_CURRENT_COLOR 0x0B00

// Primitive types
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007

// Enable caps
#define GL_TEXTURE_2D 0x0DE1
#define GL_BLEND 0x0BE2
#define GL_ALPHA_TEST 0x0BC0
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44
#define GL_FOG 0x0B60
#define GL_LIGHTING 0x0B50
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_COLOR_MATERIAL 0x0B57
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_COLOR_LOGIC_OP 0x0BF2

// GL_RESCALE_NORMAL (GL_EXT_rescale_normal = 32826 = 0x803A)
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 0x803A
#endif
#define GL_RESCALE_NORMAL_EXT GL_RESCALE_NORMAL
#define GL_NORMALIZE 0x0BA1

// Debug (GL 4.3+)
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B

// Blend factors
#define GL_ZERO 0
#define GL_ONE 1
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307

// Comparison functions
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207

// Clear bits
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000

// Texture parameters
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803

// Texture filter values
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703

// Texture wrap values
#define GL_CLAMP 0x2900
#define GL_REPEAT 0x2901

// Pixel format
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_BGR_EXT 0x80E0

// Data types
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

// Vertex arrays
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078

// VBO
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8

// VBO ARB
#define GL_ARRAY_BUFFER_ARB 0x8892
#define GL_STREAM_DRAW_ARB 0x88E0

// Fog
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_FOG_MODE 0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_EXP 0x0800
#define GL_EXP2 0x0801
// GL_LINEAR already defined above as 0x2601

// Lighting
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_POSITION 0x1203
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_LIGHT_MODEL_AMBIENT 0x0B53

// Shade model
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01

// Logic op
#define GL_COPY 0x1503
#define GL_OR_REVERSE 0x150B

// Display list
#define GL_COMPILE 0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301

// Pixel storage
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5

// String queries
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03

// NV fog distance mode extension
#define GL_FOG_DISTANCE_MODE_NV 0x855A
#define GL_EYE_RADIAL_NV 0x855B

// Error codes
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505

// ============================================================================
// Rendering Backend Function Declarations
// ============================================================================

// --- Matrix Stack ---
void glMatrixMode(GLenum mode);
void glLoadIdentity();
void glPushMatrix();
void glPopMatrix();
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScaled(GLdouble x, GLdouble y, GLdouble z);
void glMultMatrixf(const GLfloat* m);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

// --- State Enable/Disable ---
void glEnable(GLenum cap);
void glDisable(GLenum cap);

// --- Blending ---
void glBlendFunc(GLenum sfactor, GLenum dfactor);

// --- Color ---
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

// --- Depth ---
void glDepthMask(GLboolean flag);
void glDepthFunc(GLenum func);
void glClearDepth(GLdouble depth);

// --- Alpha ---
void glAlphaFunc(GLenum func, GLclampf ref);

// --- Fog ---
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat* params);
void glFogi(GLenum pname, GLint param);

// --- Lighting ---
void glLightfv(GLenum light, GLenum pname, const GLfloat* params);
void glLightModelfv(GLenum pname, const GLfloat* params);
void glColorMaterial(GLenum face, GLenum mode);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glShadeModel(GLenum mode);

// --- Clear / Viewport ---
void glClear(GLbitfield mask);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// --- Textures ---
void glGenTextures(GLsizei n, GLuint* textures);
void glDeleteTextures(GLsizei n, const GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const void* pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
void glPixelStorei(GLenum pname, GLint param);

// --- Display Lists ---
GLuint glGenLists(GLsizei range);
void glDeleteLists(GLuint list, GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList();
void glCallList(GLuint list);
void glCallLists(GLsizei n, GLenum type, const void* lists);

// --- Vertex Arrays ---
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void glNormalPointer(GLenum type, GLsizei stride, const void* pointer);
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

// --- VBO (GL 1.5) ---
void glGenBuffers(GLsizei n, GLuint* buffers);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);

// --- VBO (ARB extension fallback) ---
void glGenBuffersARB(GLsizei n, GLuint* buffers);
void glBindBufferARB(GLenum target, GLuint buffer);
void glBufferDataARB(GLenum target, GLsizeiptr size, const void* data, GLenum usage);

// --- Misc State ---
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glCullFace(GLenum mode);
void glLineWidth(GLfloat width);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glLogicOp(GLenum opcode);

// --- Query / Debug ---
void glGetFloatv(GLenum pname, GLfloat* params);
GLenum glGetError();
const GLubyte* glGetString(GLenum name);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
void glDebugMessageCallback(GLDEBUGPROC callback, const void* userParam);

// ============================================================================
// GLU Utility Functions
// ============================================================================

void gluPerspective(float fovy, float aspect, float zNear, float zFar);

// ============================================================================
// Backend Feature Queries
// ============================================================================

bool RenderBackend_SupportsVBO();
bool RenderBackend_SupportsARBVBO();
