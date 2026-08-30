#pragma once

// LegacyGL frontend.
//
// This header is the single compatibility-OpenGL surface the game compiles
// against. Its contents are derived from the GL/GLU entry points and enums that
// current main actually issues; see docs/portable/gl-api-coverage.md. The
// declarations intentionally mirror OpenGL 1.1 (plus GL_ARB_vertex_buffer_object
// and the two rescale-normal spellings the renderer uses) so that no call site
// has to change.
//
// Nothing here includes a GL loader. The semantic core (legacygl/Context) owns
// legacy state, and one backend sink (legacygl/Sink.h) performs the actual
// rendering. The native compatibility sink is the behavioural oracle.
//
// A translation unit must never see both this header and a GL loader header:
// loaders redefine gl* as macros, which would silently bypass the frontend.

#if defined(__glad_h_) || defined(__gl_h_) || defined(__GL_H__) || defined(GL_VERSION_1_1)
#error "legacygl/LegacyGL.h and a GL loader header cannot appear in the same translation unit"
#endif

#include <cstddef>

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
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
typedef std::ptrdiff_t GLintptr;
typedef std::ptrdiff_t GLsizeiptr;
typedef std::ptrdiff_t GLintptrARB;
typedef std::ptrdiff_t GLsizeiptrARB;

// Boolean
#define GL_FALSE 0
#define GL_TRUE 1

// Errors
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505

// Primitives
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009

// Data types
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_DOUBLE 0x140A

// Buffer clear bits
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000

// Comparison functions (depth test, alpha test)
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207

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
#define GL_SRC_ALPHA_SATURATE 0x0308

// Logic operations
#define GL_CLEAR 0x1500
#define GL_AND 0x1501
#define GL_AND_REVERSE 0x1502
#define GL_COPY 0x1503
#define GL_AND_INVERTED 0x1504
#define GL_NOOP 0x1505
#define GL_XOR 0x1506
#define GL_OR 0x1507
#define GL_NOR 0x1508
#define GL_EQUIV 0x1509
#define GL_INVERT 0x150A
#define GL_OR_REVERSE 0x150B
#define GL_COPY_INVERTED 0x150C
#define GL_OR_INVERTED 0x150D
#define GL_NAND 0x150E
#define GL_SET 0x150F

// Faces and winding
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_CW 0x0900
#define GL_CCW 0x0901

// Server enables
#define GL_LINE_SMOOTH 0x0B20
#define GL_CULL_FACE 0x0B44
#define GL_LIGHTING 0x0B50
#define GL_COLOR_MATERIAL 0x0B57
#define GL_FOG 0x0B60
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_NORMALIZE 0x0BA1
#define GL_ALPHA_TEST 0x0BC0
#define GL_DITHER 0x0BD0
#define GL_BLEND 0x0BE2
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_SCISSOR_TEST 0x0C11
#define GL_TEXTURE_2D 0x0DE1
#define GL_PROXY_TEXTURE_2D 0x8064
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_RESCALE_NORMAL 0x803A
#define GL_RESCALE_NORMAL_EXT 0x803A
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_LIGHT2 0x4002
#define GL_LIGHT3 0x4003
#define GL_LIGHT4 0x4004
#define GL_LIGHT5 0x4005
#define GL_LIGHT6 0x4006
#define GL_LIGHT7 0x4007

// Shade model
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01

// Matrix modes
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702

// Fog
#define GL_FOG_INDEX 0x0B61
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_FOG_MODE 0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_EXP 0x0800
#define GL_EXP2 0x0801
#define GL_LINEAR 0x2601

// GL_NV_fog_distance: Alpha selects radial eye distance for its nether fog.
#define GL_FOG_DISTANCE_MODE_NV 0x855A
#define GL_EYE_RADIAL_NV 0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV 0x855C
#define GL_EYE_PLANE 0x2502

// Lighting and material parameters
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_POSITION 0x1203
#define GL_SPOT_DIRECTION 0x1204
#define GL_SPOT_EXPONENT 0x1205
#define GL_SPOT_CUTOFF 0x1206
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_EMISSION 0x1600
#define GL_SHININESS 0x1601
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_LIGHT_MODEL_TWO_SIDE 0x0B52
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x0B51
#define GL_LIGHT_MODEL_AMBIENT 0x0B53

// Texture parameters and formats
#define GL_NEAREST 0x2600
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_CLAMP 0x2900
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_BGR_EXT 0x80E0
#define GL_BGRA_EXT 0x80E1
#define GL_ALPHA 0x1906
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A

// Pixel storage
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05

// Client arrays
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078

// Display lists
#define GL_COMPILE 0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301

// Queries
#define GL_CURRENT_COLOR 0x0B00
#define GL_CURRENT_NORMAL 0x0B02
#define GL_CURRENT_TEXTURE_COORDS 0x0B03
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_TEXTURE_MATRIX 0x0BA8
#define GL_VIEWPORT 0x0BA2
#define GL_LINE_WIDTH 0x0B21
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_ALPHA_TEST_REF 0x0BC2

// GL_ARB_vertex_buffer_object
#define GL_ARRAY_BUFFER_ARB 0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#define GL_STREAM_DRAW_ARB 0x88E0
#define GL_STATIC_DRAW_ARB 0x88E4
#define GL_DYNAMIC_DRAW_ARB 0x88E8

// Matrix stack
void glMatrixMode(GLenum mode);
void glLoadIdentity();
void glPushMatrix();
void glPopMatrix();
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScaled(GLdouble x, GLdouble y, GLdouble z);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

// Server state
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glAlphaFunc(GLenum func, GLclampf ref);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glCullFace(GLenum mode);
void glShadeModel(GLenum mode);
void glLogicOp(GLenum opcode);
void glLineWidth(GLfloat width);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glPixelStorei(GLenum pname, GLint param);

// Current attributes
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz);

// Fog
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi(GLenum pname, GLint param);

// Lighting
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightModelfv(GLenum pname, const GLfloat *params);
void glColorMaterial(GLenum face, GLenum mode);

// Textures
void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
	GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const GLvoid *pixels);

// Client arrays
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

// Immediate mode
void glBegin(GLenum mode);
void glEnd();
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glTexCoord2f(GLfloat s, GLfloat t);

// GL_ARB_vertex_buffer_object
void glGenBuffersARB(GLsizei n, GLuint *buffers);
void glBindBufferARB(GLenum target, GLuint buffer);
void glBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);

// Display lists
GLuint glGenLists(GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList();
void glCallList(GLuint list);
void glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void glDeleteLists(GLuint list, GLsizei range);

// Framebuffer
void glClear(GLbitfield mask);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampd depth);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void glFinish();

// Queries
void glGetFloatv(GLenum pname, GLfloat *params);
GLenum glGetError();
