#include "BGFXCompat.h"

#ifdef USE_BGFX

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace lwjgl
{
namespace BGFXCompat
{

// Vertex layout matching Tesselator format (32 bytes per vertex):
// Position (3 floats = 12 bytes) at offset 0
// TexCoord (2 floats = 8 bytes) at offset 12
// Color (4 bytes = RGBA) at offset 20
// Normal (packed into 1 int, 4 bytes) at offset 24
// Total = 32 bytes

static bgfx::VertexLayout s_vertexLayout;

void initVertexLayout()
{
	s_vertexLayout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true) // Normalized
		.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Int8, true) // Normalized
		.end();
}

// Display list storage
static std::unordered_map<GLuint, std::unique_ptr<DisplayList>> s_displayLists;
static GLuint s_nextListId = 1;

// Global GL state
static GLState s_glState;
static uint16_t s_viewId = 0;

// Matrix stack (proper implementation)
static std::vector<std::vector<float>> s_matrixStack;

// Default shader program (will need to create one matching OpenGL 1.1 fixed function)
static bgfx::ProgramHandle s_defaultProgram = BGFX_INVALID_HANDLE;

GLState& getGLState()
{
	return s_glState;
}

// Matrix helpers
static void identityMatrix(float* m)
{
	memset(m, 0, 16 * sizeof(float));
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void multiplyMatrix(float* result, const float* a, const float* b)
{
	// Column-major matrix multiplication (OpenGL style)
	float temp[16];
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			temp[i * 4 + j] = 0.0f;
			for (int k = 0; k < 4; k++)
			{
				temp[i * 4 + j] += a[k * 4 + j] * b[i * 4 + k];
			}
		}
	}
	memcpy(result, temp, 16 * sizeof(float));
}

static void translateMatrix(float* m, float x, float y, float z)
{
	float trans[16];
	identityMatrix(trans);
	trans[12] = x;
	trans[13] = y;
	trans[14] = z;
	multiplyMatrix(m, m, trans);
}

static void scaleMatrix(float* m, float x, float y, float z)
{
	float scale[16];
	identityMatrix(scale);
	scale[0] = x;
	scale[5] = y;
	scale[10] = z;
	multiplyMatrix(m, m, scale);
}

void init()
{
	initVertexLayout();
	identityMatrix(s_glState.currentMatrix);
	s_matrixStack.clear();
	s_matrixStack.push_back(std::vector<float>(16));
	identityMatrix(s_matrixStack.back().data());
	
	// TODO: Create default shader program that matches OpenGL 1.1 fixed function pipeline
	// For now, we'll use a placeholder - you'll need to create shaders that match your rendering
	s_viewId = 0;
}

void shutdown()
{
	// Clean up display lists
	s_displayLists.clear();
	
	if (bgfx::isValid(s_defaultProgram))
	{
		bgfx::destroy(s_defaultProgram);
		s_defaultProgram = BGFX_INVALID_HANDLE;
	}
}

// Display list functions
GLuint genLists(GLsizei range)
{
	if (range <= 0)
		return 0;
	
	// Allocate range of list IDs starting from s_nextListId
	GLuint base = s_nextListId;
	for (GLsizei i = 0; i < range; i++)
	{
		s_displayLists[s_nextListId + i] = std::make_unique<DisplayList>();
	}
	s_nextListId += range;
	return base;
}

void newList(GLuint list, GLenum mode)
{
	s_glState.compilingList = true;
	s_glState.currentList = list;
	s_glState.currentDrawMode = mode;
	s_glState.currentVertices.clear();
	s_glState.currentHasTexture = false;
	s_glState.currentHasColor = false;
	s_glState.currentHasNormal = false;
	
	// Ensure list exists
	if (s_displayLists.find(list) == s_displayLists.end())
	{
		s_displayLists[list] = std::make_unique<DisplayList>();
	}
}

void endList()
{
	if (!s_glState.compilingList)
		return;
	
	s_glState.compilingList = false;
	
	// Convert collected vertices to bgfx vertex buffer
	GLuint list = s_glState.currentList;
	auto& dl = s_displayLists[list];
	
	if (s_glState.currentVertices.empty())
	{
		// Empty list
		dl->numVertices = 0;
		return;
	}
	
	// Convert QUADS to TRIANGLES if needed
	std::vector<float> finalVertices;
	GLenum mode = s_glState.currentDrawMode;
	size_t vertexCount = s_glState.currentVertices.size() / 8; // 8 floats per vertex
	
	if (mode == GL_QUADS && vertexCount >= 4)
	{
		// Convert quads to triangles
		for (size_t i = 0; i < vertexCount; i += 4)
		{
			if (i + 3 < vertexCount)
			{
				// Quad: v0, v1, v2, v3 -> Triangles: (v0,v1,v2), (v0,v2,v3)
				size_t base = i * 8;
				// Triangle 1: v0, v1, v2
				finalVertices.insert(finalVertices.end(), 
					s_glState.currentVertices.begin() + base,
					s_glState.currentVertices.begin() + base + 8);
				finalVertices.insert(finalVertices.end(),
					s_glState.currentVertices.begin() + base + 8,
					s_glState.currentVertices.begin() + base + 16);
				finalVertices.insert(finalVertices.end(),
					s_glState.currentVertices.begin() + base + 16,
					s_glState.currentVertices.begin() + base + 24);
				// Triangle 2: v0, v2, v3
				finalVertices.insert(finalVertices.end(),
					s_glState.currentVertices.begin() + base,
					s_glState.currentVertices.begin() + base + 8);
				finalVertices.insert(finalVertices.end(),
					s_glState.currentVertices.begin() + base + 16,
					s_glState.currentVertices.begin() + base + 24);
				finalVertices.insert(finalVertices.end(),
					s_glState.currentVertices.begin() + base + 24,
					s_glState.currentVertices.begin() + base + 32);
			}
		}
		vertexCount = finalVertices.size() / 8;
	}
	else
	{
		finalVertices = s_glState.currentVertices;
	}
	
	// Create bgfx vertex buffer
	if (vertexCount > 0)
	{
		const bgfx::Memory* mem = bgfx::makeRef(finalVertices.data(), 
			static_cast<uint32_t>(finalVertices.size() * sizeof(float)));
		
		dl->vb = bgfx::createVertexBuffer(mem, s_vertexLayout);
		dl->numVertices = static_cast<uint32_t>(vertexCount);
		dl->hasTexture = s_glState.currentHasTexture;
		dl->hasColor = s_glState.currentHasColor;
		dl->hasNormal = s_glState.currentHasNormal;
	}
}

void callList(GLuint list)
{
	auto it = s_displayLists.find(list);
	if (it == s_displayLists.end() || !it->second || it->second->numVertices == 0)
		return;
	
	auto& dl = *it->second;
	
	// Set vertex buffer
	bgfx::setVertexBuffer(0, dl.vb);
	
	// Set state
	uint64_t state = 0;
	if (s_glState.depthTest)
		state |= BGFX_STATE_DEPTH_TEST_LESS;
	if (s_glState.cullFace)
		state |= BGFX_STATE_CULL_CCW;
	if (s_glState.blend)
		state |= s_glState.blendState;
	else
		state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
	
	// Set texture if available
	if (bgfx::isValid(s_glState.currentTexture))
	{
		bgfx::setTexture(0, bgfx::UniformHandle{}, s_glState.currentTexture);
	}
	
	// Set transform matrix
	float transform[16];
	memcpy(transform, s_glState.currentMatrix, 16 * sizeof(float));
	bgfx::setTransform(transform);
	
	// Submit (need shader program - TODO)
	// For now, we'll need a default shader program
	// bgfx::submit(s_viewId, s_defaultProgram);
}

void callLists(GLsizei n, GLenum type, const GLvoid* lists)
{
	if (!lists) return;
	
	const GLuint* listArray = reinterpret_cast<const GLuint*>(lists);
	for (GLsizei i = 0; i < n; i++)
	{
		GLuint listId = listArray[i];
		callList(listId);
	}
}

// Matrix functions
void pushMatrix()
{
	// Push current matrix to stack
	std::vector<float> matrix(16);
	memcpy(matrix.data(), s_glState.currentMatrix, 16 * sizeof(float));
	s_matrixStack.push_back(matrix);
}

void popMatrix()
{
	if (s_matrixStack.size() <= 1)
		return;
	
	// Pop from stack and restore
	s_matrixStack.pop_back();
	memcpy(s_glState.currentMatrix, s_matrixStack.back().data(), 16 * sizeof(float));
}

void loadIdentity()
{
	identityMatrix(s_glState.currentMatrix);
}

void translatef(GLfloat x, GLfloat y, GLfloat z)
{
	translateMatrix(s_glState.currentMatrix, x, y, z);
}

void scalef(GLfloat x, GLfloat y, GLfloat z)
{
	scaleMatrix(s_glState.currentMatrix, x, y, z);
}

void rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	// TODO: Implement rotation matrix
}

void multMatrixf(const GLfloat* m)
{
	multiplyMatrix(s_glState.currentMatrix, s_glState.currentMatrix, m);
}

// State functions (stubs for now)
void enable(GLenum cap)
{
	switch (cap)
	{
	case GL_DEPTH_TEST:
		s_glState.depthTest = true;
		break;
	case GL_BLEND:
		s_glState.blend = true;
		break;
	case GL_CULL_FACE:
		s_glState.cullFace = true;
		break;
	case GL_ALPHA_TEST:
		s_glState.alphaTest = true;
		break;
	}
}

void disable(GLenum cap)
{
	switch (cap)
	{
	case GL_DEPTH_TEST:
		s_glState.depthTest = false;
		break;
	case GL_BLEND:
		s_glState.blend = false;
		break;
	case GL_CULL_FACE:
		s_glState.cullFace = false;
		break;
	case GL_ALPHA_TEST:
		s_glState.alphaTest = false;
		break;
	}
}

void blendFunc(GLenum sfactor, GLenum dfactor)
{
	// Map OpenGL blend functions to bgfx state
	s_glState.blendState = BGFX_STATE_BLEND_ALPHA; // Default
}

void alphaFunc(GLenum func, GLclampf ref) { }
void depthFunc(GLenum func) { }
void cullFace(GLenum mode) { }

// Viewport and clear
void viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	bgfx::setViewRect(s_viewId, static_cast<uint16_t>(x), static_cast<uint16_t>(y),
		static_cast<uint16_t>(width), static_cast<uint16_t>(height));
}

void clear(GLbitfield mask)
{
	uint16_t flags = 0;
	if (mask & GL_COLOR_BUFFER_BIT)
		flags |= BGFX_CLEAR_COLOR;
	if (mask & GL_DEPTH_BUFFER_BIT)
		flags |= BGFX_CLEAR_DEPTH;
	if (mask & GL_STENCIL_BUFFER_BIT)
		flags |= BGFX_CLEAR_STENCIL;
	
	bgfx::setViewClear(s_viewId, flags, 0, 1.0f, 0);
}

void clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	uint32_t rgba = (static_cast<uint32_t>(alpha * 255) << 24) |
		(static_cast<uint32_t>(blue * 255) << 16) |
		(static_cast<uint32_t>(green * 255) << 8) |
		(static_cast<uint32_t>(red * 255));
	bgfx::setViewClear(s_viewId, BGFX_CLEAR_COLOR, rgba, 1.0f, 0);
}

void clearDepth(GLclampd depth) { }

void matrixMode(GLenum mode) { }
void loadMatrixf(const GLfloat* m)
{
	memcpy(s_glState.currentMatrix, m, 16 * sizeof(float));
}

void bindTexture(GLenum target, GLuint texture)
{
	// Map OpenGL texture ID to bgfx texture handle
	// This requires a texture ID mapping system
	// For now, assume texture IDs are bgfx handles (they're not, but this needs texture loading integration)
	s_glState.currentTexture = { static_cast<uint16_t>(texture) };
}

void texParameteri(GLenum target, GLenum pname, GLint param) { }

void fogf(GLenum pname, GLfloat param) { }
void fogfv(GLenum pname, const GLfloat* params) { }
void fogi(GLenum pname, GLint param) { }

void shadeModel(GLenum mode) { }

// Vertex array functions - these are captured during display list compilation
void vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
	// During display list compilation, we capture vertex data
	// This is called by Tesselator::end()
}

void texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) { }
void colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) { }
void normalPointer(GLenum type, GLsizei stride, const GLvoid* pointer) { }

void enableClientState(GLenum cap) { }
void disableClientState(GLenum cap) { }

// Global flag to check if compiling display list (C-linkage for Tesselator)
extern "C" {
	bool bgfx_compiling_display_list()
	{
		return s_glState.compilingList;
	}
}

// Capture vertex data from Tesselator during display list compilation (C-linkage)
extern "C" void bgfx_capture_tesselator_data(const float* vertexData, size_t vertexCount, 
	GLenum drawMode, bool hasTexture, bool hasColor, bool hasNormal)
{
	if (!s_glState.compilingList || !vertexData || vertexCount == 0)
		return;
	
	// Store vertex data in current display list
	size_t dataSize = vertexCount * 8; // 8 floats per vertex
	s_glState.currentVertices.insert(s_glState.currentVertices.end(), 
		vertexData, vertexData + dataSize);
	s_glState.currentDrawMode = drawMode;
	s_glState.currentHasTexture = hasTexture;
	s_glState.currentHasColor = hasColor;
	s_glState.currentHasNormal = hasNormal;
}

// C++ wrapper
void captureTesselatorData(const float* vertexData, size_t vertexCount, GLenum drawMode,
	bool hasTexture, bool hasColor, bool hasNormal)
{
	bgfx_capture_tesselator_data(vertexData, vertexCount, drawMode, hasTexture, hasColor, hasNormal);
}

// Capture vertex data during display list compilation
// This function is called when Tesselator::end() calls glDrawArrays
void drawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (!s_glState.compilingList)
	{
		// Not compiling a list - use real OpenGL (fallback, or this shouldn't happen with bgfx)
		return;
	}
	
	// During display list compilation, vertex data should already be captured
	// via captureTesselatorData() before glDrawArrays is called
	// This function is just a marker that rendering would occur
}

} // namespace BGFXCompat
} // namespace lwjgl

#endif // USE_BGFX
