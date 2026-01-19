#pragma once

#ifdef USE_BGFX

#include <bgfx/bgfx.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace lwjgl
{
namespace BGFXCompat
{

// OpenGL display list emulation using bgfx static vertex buffers
struct DisplayList
{
	bgfx::VertexBufferHandle vb = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle ib = BGFX_INVALID_HANDLE;
	uint32_t numVertices = 0;
	uint32_t numIndices = 0;
	bool hasTexture = false;
	bool hasColor = false;
	bool hasNormal = false;
	
	~DisplayList()
	{
		if (bgfx::isValid(vb))
			bgfx::destroy(vb);
		if (bgfx::isValid(ib))
			bgfx::destroy(ib);
	}
};

// Current OpenGL state (mapped to bgfx)
struct GLState
{
	// Matrix stack (for glPushMatrix/glPopMatrix/glTranslatef)
	std::vector<float> matrixStack;
	float currentMatrix[16]; // Column-major 4x4 matrix (OpenGL style)
	
	// Rendering state
	bool depthTest = true;
	bool blend = false;
	bool cullFace = true;
	bool alphaTest = false;
	
	// Blending
	uint64_t blendState = 0;
	
	// Texture
	bgfx::TextureHandle currentTexture = BGFX_INVALID_HANDLE;
	
	// Current display list being built
	GLuint currentList = 0;
	std::vector<float> currentVertices; // xyz uvw rgba normal
	uint32_t currentVertexStride = 32; // 8 floats = 32 bytes
	bool compilingList = false;
	bool currentHasTexture = false;
	bool currentHasColor = false;
	bool currentHasNormal = false;
	GLenum currentDrawMode = 0;
};

// Get global GL state
GLState& getGLState();

// Display list management
GLuint genLists(GLsizei range);
void newList(GLuint list, GLenum mode);
void endList();
void callList(GLuint list);
void callLists(GLsizei n, GLenum type, const GLvoid* lists);

// Vertex array functions (mapped to bgfx)
void vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
void texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
void colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
void normalPointer(GLenum type, GLsizei stride, const GLvoid* pointer);
void enableClientState(GLenum cap);
void disableClientState(GLenum cap);
void drawArrays(GLenum mode, GLint first, GLsizei count);

// Matrix functions
void pushMatrix();
void popMatrix();
void loadIdentity();
void translatef(GLfloat x, GLfloat y, GLfloat z);
void scalef(GLfloat x, GLfloat y, GLfloat z);
void rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void multMatrixf(const GLfloat* m);

// State functions
void enable(GLenum cap);
void disable(GLenum cap);
void blendFunc(GLenum sfactor, GLenum dfactor);
void alphaFunc(GLenum func, GLclampf ref);
void depthFunc(GLenum func);
void cullFace(GLenum mode);

// Viewport and clear
void viewport(GLint x, GLint y, GLsizei width, GLsizei height);
void clear(GLbitfield mask);
void clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void clearDepth(GLclampd depth);

// Matrix mode (stored but matrices managed separately)
void matrixMode(GLenum mode);
void loadMatrixf(const GLfloat* m);

// Texture functions
void bindTexture(GLenum target, GLuint texture);
void texParameteri(GLenum target, GLenum pname, GLint param);

// Fog
void fogf(GLenum pname, GLfloat param);
void fogfv(GLenum pname, const GLfloat* params);
void fogi(GLenum pname, GLint param);

// Shade model (smooth/flat)
void shadeModel(GLenum mode);

// Initialize bgfx compatibility layer
void init();

// Shutdown
void shutdown();

// Capture vertex data from Tesselator during display list compilation
// This is called by Tesselator when glDrawArrays is invoked during list compilation
void captureTesselatorData(const float* vertexData, size_t vertexCount, GLenum drawMode, 
	bool hasTexture, bool hasColor, bool hasNormal);

} // namespace BGFXCompat
} // namespace lwjgl

#endif // USE_BGFX
