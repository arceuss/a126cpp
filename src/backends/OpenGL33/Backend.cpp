// OpenGL 3.3 Core backend for the LegacyGL semantic frontend.
//
// This file consumes only resolved commands. The raw Sink callbacks remain
// no-ops (apart from object-name allocation and glFinish), so fixed-function
// state is owned and interpreted exactly once by Context.

#include <glad/glad.h>

#include "backends/Backend.h"
#include "backends/OpenGL/Context.h"
#include "backends/OpenGL33/Shaders.h"
#include "legacygl/PhaseProfile.h"
#include "legacygl/Sink.h"
#include "legacygl/PixelFormat.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace legacygl
{

static const int CORE_TEXTURE_LEVELS = 16;

// The vertex the shader consumes: position, colour, normal, texcoord, 48
// bytes. Flat shading reads attribute locations 4/5/6, which the vertex array
// aliases onto these same fields; the provoking vertex is emitted last in every
// triangle and glProvokingVertex(GL_LAST_VERTEX_CONVENTION) makes the flat
// varying take its value from that vertex, so no duplicated copy is needed.
struct CoreGPUVertex
{
	float position[3];
	float color[4];
	float normal[3];
	float texCoord[2];
	// Slot of this vertex's resident entry within its page, or 0 when the
	// draw is not batched. Stamped at resident upload, read by the vertex
	// shader only when a batch is being drawn.
	std::uint32_t drawSlot;
};

// The one source mode rotation cannot serve: a legacy quad's provoking vertex
// is its fourth, which the first of its two triangles does not contain. Those
// draws keep an explicit copy of the provoking attributes per vertex, 88 bytes.
// No game code submits GL_QUADS (Tesselator converts to triangles first), so
// this layout exists for the layer's generality rather than the game's paths.
struct CoreGPUFlatVertex
{
	float position[3];
	float color[4];
	float normal[3];
	float texCoord[2];
	float flatPosition[3];
	float flatColor[4];
	float flatNormal[3];
	std::uint32_t drawSlot;
};

enum class CoreVertexLayout
{
	Compact,
	Expanded
};

static GLsizei coreVertexStride(CoreVertexLayout layout)
{
	return layout == CoreVertexLayout::Compact ?
		static_cast<GLsizei>(sizeof(CoreGPUVertex)) :
		static_cast<GLsizei>(sizeof(CoreGPUFlatVertex));
}

// Vertex data ready for upload, in whichever layout the batch needs.
struct CoreVertexData
{
	CoreVertexLayout layout = CoreVertexLayout::Compact;
	std::size_t count = 0;
	std::vector<unsigned char> bytes;

	GLsizeiptr byteSize() const { return static_cast<GLsizeiptr>(bytes.size()); }
};

// Page-backed residency, for the reason measured on GL2.1: a vertex array per
// display list makes every draw rebind, and that one call dominated the draw
// cost. Pages are keyed by attribute set and vertex layout because both are
// vertex-array state; within a page the stride is fixed, so it can hold any
// geometry sharing that key and the draw supplies a first-vertex.
struct CoreResidentFreeRange
{
	GLsizeiptr offset = 0;
	GLsizeiptr size = 0;
};

// Matrix slots per batch block. GL 3.3 guarantees only 16 KiB per uniform
// block; 128 model-view + 128 normal matrices is exactly that. Entries whose
// page slot is at or above this draw unbatched.
static const std::size_t CORE_BATCH_SLOTS = 128;
// Batch blocks per ring region; a frame issuing more page runs than this
// drains the GPU, like the state ring does on overflow.
static const std::size_t CORE_BATCH_BLOCKS = 512;

struct CoreBatchedDraw
{
	std::size_t page = 0;
	std::uint32_t slot = 0;
	GLint first = 0;
	GLsizei count = 0;
	Mat4 modelView;
	Mat4 normal;
};

// std140 image of LegacyBatchBlock: two mat4 arrays, 16 KiB.
struct alignas(16) CoreBatchBlock
{
	Mat4 modelView[CORE_BATCH_SLOTS];
	Mat4 normal[CORE_BATCH_SLOTS];
};
static_assert(sizeof(CoreBatchBlock) == 16384, "Batch block must be the 16 KiB GL 3.3 minimum");

struct CoreResidentPage
{
	GLuint buffer = 0;
	GLuint vertexArray = 0;
	unsigned int attributeMask = 0;
	CoreVertexLayout layout = CoreVertexLayout::Compact;
	GLsizeiptr capacity = 0;
	std::size_t liveEntries = 0;
	std::vector<CoreResidentFreeRange> freeRanges;
	// Slot ids are dense per page so a batch block indexes by them. Slots
	// beyond CORE_BATCH_SLOTS exist (the page can hold more entries) but
	// those entries draw unbatched.
	std::vector<std::uint32_t> freeSlots;
	std::uint32_t nextSlot = 0;
};

struct CoreResidentGeometryEntry
{
	std::size_t page = 0;
	GLsizeiptr byteOffset = 0;
	GLsizeiptr byteSize = 0;
	GLint firstVertex = 0;
	std::uint32_t slot = 0;
	Topology topology = Topology::Triangles;
	std::size_t vertexCount = 0;
	bool hasColor = false;
	bool hasNormal = false;
	bool hasTexCoord = false;
};

static const GLsizeiptr CORE_RESIDENT_PAGE_BYTES = 64 * 1024 * 1024;

struct alignas(16) CoreGPUMaterial
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float emission[4];
	float shininess[4];
};

struct alignas(16) CoreGPULight
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float positionEye[4];
	float spotDirectionCutoff[4];
	float attenuationExponent[4];
};

struct alignas(16) CoreGPUState
{
	float modelView[16];
	float projection[16];
	float texture[16];
	float normal[16];
	float globalAmbient[4];
	CoreGPUMaterial frontMaterial;
	CoreGPUMaterial backMaterial;
	CoreGPULight lights[8];
	float fogColor[4];
	float fogParams[4];
	float textureSize[4];
	float normalParams[4];
	unsigned int flags0[4];
	unsigned int flags1[4];
	unsigned int flags2[4];
	unsigned int flags3[4];
};

struct CoreTextureLevel
{
	int width = 0;
	int height = 0;
	bool defined = false;
	std::vector<unsigned char> rgba;
};

struct CoreTexture
{
	GLuint handle = 0;
	GLuint sampler = 0;
	CoreTextureLevel levels[CORE_TEXTURE_LEVELS];
	bool derivedDirty = true;
	bool derivedHasGutter = false;
	unsigned int derivedWrapS = 0;
	unsigned int derivedWrapT = 0;
	unsigned char derivedBorder[4] = { 0, 0, 0, 0 };
	// Last values pushed into the sampler object. Zero is not a legal filter or
	// wrap enum, so the first use always writes.
	GLenum samplerMinFilter = 0;
	GLenum samplerMagFilter = 0;
	GLenum samplerWrapS = 0;
	GLenum samplerWrapT = 0;
};

// How the per-draw uniform block reaches the GPU. Selectable so the choice
// stays a measurement on real hardware rather than an assumption.
enum class CoreUniformMode
{
	Persistent,
	Legacy
};

static const std::size_t CORE_UNIFORM_REGIONS = 3;
// Transient vertex data streams through a persistently mapped ring with the
// same three regions and fences as the uniforms, for the reason measured on
// the Switch: glBufferData per transient draw cost 26us there, the buffer
// store being reallocated every call. A draw larger than one region takes a
// separate mutable buffer; nothing the game draws transiently comes close.
static const GLsizeiptr CORE_STREAM_REGION_BYTES = 4 * 1024 * 1024;

class CoreLogicalNameAllocator
{
public:
	unsigned int allocate()
	{
		if (names.size() == static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
			return 0;
		while (names.find(nextName) != names.end())
			advance();

		const unsigned int name = nextName;
		names.insert(name);
		advance();
		return name;
	}

	void reserve(unsigned int name)
	{
		if (name != 0)
			names.insert(name);
	}

	void release(unsigned int name)
	{
		if (name != 0)
			names.erase(name);
	}

private:
	void advance()
	{
		nextName = nextName == std::numeric_limits<unsigned int>::max() ? 1 : nextName + 1;
	}

	std::set<unsigned int> names;
	unsigned int nextName = 1;
};

static_assert(sizeof(unsigned int) == 4, "Core shader flags require 32-bit unsigned int");
static_assert(sizeof(CoreGPUMaterial) == 80, "Core shader material ABI changed");
static_assert(offsetof(CoreGPUMaterial, ambient) == 0, "Core shader material ambient ABI changed");
static_assert(offsetof(CoreGPUMaterial, diffuse) == 16, "Core shader material diffuse ABI changed");
static_assert(offsetof(CoreGPUMaterial, specular) == 32, "Core shader material specular ABI changed");
static_assert(offsetof(CoreGPUMaterial, emission) == 48, "Core shader material emission ABI changed");
static_assert(offsetof(CoreGPUMaterial, shininess) == 64, "Core shader material shininess ABI changed");
static_assert(sizeof(CoreGPULight) == 96, "Core shader light ABI changed");
static_assert(offsetof(CoreGPULight, ambient) == 0, "Core shader light ambient ABI changed");
static_assert(offsetof(CoreGPULight, diffuse) == 16, "Core shader light diffuse ABI changed");
static_assert(offsetof(CoreGPULight, specular) == 32, "Core shader light specular ABI changed");
static_assert(offsetof(CoreGPULight, positionEye) == 48, "Core shader light position ABI changed");
static_assert(offsetof(CoreGPULight, spotDirectionCutoff) == 64, "Core shader light spot ABI changed");
static_assert(offsetof(CoreGPULight, attenuationExponent) == 80, "Core shader light attenuation ABI changed");
static_assert(offsetof(CoreGPUState, modelView) == 0, "Core shader model-view ABI changed");
static_assert(offsetof(CoreGPUState, projection) == 64, "Core shader projection ABI changed");
static_assert(offsetof(CoreGPUState, texture) == 128, "Core shader texture matrix ABI changed");
static_assert(offsetof(CoreGPUState, normal) == 192, "Core shader normal matrix ABI changed");
static_assert(offsetof(CoreGPUState, globalAmbient) == 256, "Core shader ambient ABI changed");
static_assert(offsetof(CoreGPUState, frontMaterial) == 272, "Core shader front material ABI changed");
static_assert(offsetof(CoreGPUState, backMaterial) == 352, "Core shader back material ABI changed");
static_assert(offsetof(CoreGPUState, lights) == 432, "Core shader light ABI changed");
static_assert(offsetof(CoreGPUState, fogColor) == 1200, "Core shader fog color ABI changed");
static_assert(offsetof(CoreGPUState, fogParams) == 1216, "Core shader fog parameters ABI changed");
static_assert(offsetof(CoreGPUState, textureSize) == 1232, "Core shader texture size ABI changed");
static_assert(offsetof(CoreGPUState, normalParams) == 1248, "Core shader normal parameters ABI changed");
static_assert(offsetof(CoreGPUState, flags0) == 1264, "Core shader flags0 ABI changed");
static_assert(offsetof(CoreGPUState, flags1) == 1280, "Core shader flags1 ABI changed");
static_assert(offsetof(CoreGPUState, flags2) == 1296, "Core shader flags2 ABI changed");
static_assert(offsetof(CoreGPUState, flags3) == 1312, "Core shader flags3 ABI changed");
static_assert(sizeof(CoreGPUState) == 1328, "Core shader block ABI changed");

struct CoreUniformMember
{
	const char *name;
	GLint offset;
};

static bool coreValidateUniformLayout(GLuint program)
{
	static const CoreUniformMember members[] = {
		{ "uModelView", static_cast<GLint>(offsetof(CoreGPUState, modelView)) },
		{ "uProjection", static_cast<GLint>(offsetof(CoreGPUState, projection)) },
		{ "uTexture", static_cast<GLint>(offsetof(CoreGPUState, texture)) },
		{ "uNormalMatrix", static_cast<GLint>(offsetof(CoreGPUState, normal)) },
		{ "uGlobalAmbient", static_cast<GLint>(offsetof(CoreGPUState, globalAmbient)) },
		{ "uFrontMaterial.ambient", static_cast<GLint>(offsetof(CoreGPUState, frontMaterial) + offsetof(CoreGPUMaterial, ambient)) },
		{ "uFrontMaterial.diffuse", static_cast<GLint>(offsetof(CoreGPUState, frontMaterial) + offsetof(CoreGPUMaterial, diffuse)) },
		{ "uFrontMaterial.specular", static_cast<GLint>(offsetof(CoreGPUState, frontMaterial) + offsetof(CoreGPUMaterial, specular)) },
		{ "uFrontMaterial.emission", static_cast<GLint>(offsetof(CoreGPUState, frontMaterial) + offsetof(CoreGPUMaterial, emission)) },
		{ "uFrontMaterial.shininess", static_cast<GLint>(offsetof(CoreGPUState, frontMaterial) + offsetof(CoreGPUMaterial, shininess)) },
		{ "uLights[0].ambient", static_cast<GLint>(offsetof(CoreGPUState, lights) + offsetof(CoreGPULight, ambient)) },
		{ "uLights[0].diffuse", static_cast<GLint>(offsetof(CoreGPUState, lights) + offsetof(CoreGPULight, diffuse)) },
		{ "uLights[0].specular", static_cast<GLint>(offsetof(CoreGPUState, lights) + offsetof(CoreGPULight, specular)) },
		{ "uLights[0].positionEye", static_cast<GLint>(offsetof(CoreGPUState, lights) + offsetof(CoreGPULight, positionEye)) },
		{ "uLights[0].spotDirectionCutoff", static_cast<GLint>(offsetof(CoreGPUState, lights) + offsetof(CoreGPULight, spotDirectionCutoff)) },
		{ "uLights[0].attenuationExponent", static_cast<GLint>(offsetof(CoreGPUState, lights) + offsetof(CoreGPULight, attenuationExponent)) },
		{ "uLights[1].ambient", static_cast<GLint>(offsetof(CoreGPUState, lights) + sizeof(CoreGPULight) + offsetof(CoreGPULight, ambient)) },
		{ "uLights[1].diffuse", static_cast<GLint>(offsetof(CoreGPUState, lights) + sizeof(CoreGPULight) + offsetof(CoreGPULight, diffuse)) },
		{ "uLights[1].specular", static_cast<GLint>(offsetof(CoreGPUState, lights) + sizeof(CoreGPULight) + offsetof(CoreGPULight, specular)) },
		{ "uLights[1].positionEye", static_cast<GLint>(offsetof(CoreGPUState, lights) + sizeof(CoreGPULight) + offsetof(CoreGPULight, positionEye)) },
		{ "uLights[1].spotDirectionCutoff", static_cast<GLint>(offsetof(CoreGPUState, lights) + sizeof(CoreGPULight) + offsetof(CoreGPULight, spotDirectionCutoff)) },
		{ "uLights[1].attenuationExponent", static_cast<GLint>(offsetof(CoreGPUState, lights) + sizeof(CoreGPULight) + offsetof(CoreGPULight, attenuationExponent)) },
		{ "uFogColor", static_cast<GLint>(offsetof(CoreGPUState, fogColor)) },
		{ "uFogParams", static_cast<GLint>(offsetof(CoreGPUState, fogParams)) },
		{ "uTextureSize", static_cast<GLint>(offsetof(CoreGPUState, textureSize)) },
		{ "uNormalParams", static_cast<GLint>(offsetof(CoreGPUState, normalParams)) },
		{ "uFlags0", static_cast<GLint>(offsetof(CoreGPUState, flags0)) },
		{ "uFlags1", static_cast<GLint>(offsetof(CoreGPUState, flags1)) },
		{ "uFlags2", static_cast<GLint>(offsetof(CoreGPUState, flags2)) },
		{ "uFlags3", static_cast<GLint>(offsetof(CoreGPUState, flags3)) }
	};
	static const int memberCount = static_cast<int>(sizeof(members) / sizeof(members[0]));
	const GLchar *names[memberCount];
	GLuint indices[memberCount];
	GLint offsets[memberCount];
	for (int i = 0; i < memberCount; i++)
		names[i] = members[i].name;

	glGetUniformIndices(program, memberCount, names, indices);
	for (int i = 0; i < memberCount; i++)
	{
		if (indices[i] == GL_INVALID_INDEX)
		{
			std::fprintf(stderr, "LegacyGL gl33: shader block ABI member '%s' is inactive or missing\n",
				members[i].name);
			return false;
		}
	}

	glGetActiveUniformsiv(program, memberCount, indices, GL_UNIFORM_OFFSET, offsets);
	for (int i = 0; i < memberCount; i++)
	{
		if (offsets[i] != members[i].offset)
		{
			std::fprintf(stderr,
				"LegacyGL gl33: shader block ABI member '%s' offset mismatch (driver=%d, cpu=%d)\n",
				members[i].name, offsets[i], members[i].offset);
			return false;
		}
	}
	return true;
}

static void coreCopy4(float *destination, const float *source)
{
	std::memcpy(destination, source, sizeof(float) * 4);
}

static unsigned char coreFloatByte(float value)
{
	value = std::max(0.0f, std::min(1.0f, value));
	return static_cast<unsigned char>(std::lround(value * 255.0f));
}

static std::size_t coreAlignedRowSize(std::size_t rowSize, int alignment)
{
	const std::size_t value = static_cast<std::size_t>(alignment);
	return (rowSize + value - 1) / value * value;
}

static GLuint coreCompileShader(GLenum type, const char *source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_TRUE)
		return shader;

	GLint length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	std::vector<char> log(static_cast<std::size_t>(std::max(length, 1)), 0);
	glGetShaderInfoLog(shader, length, nullptr, log.data());
	std::fprintf(stderr, "LegacyGL gl33 shader compile failed: %s\n", log.data());
	glDeleteShader(shader);
	return 0;
}

static GLuint coreCreateProgram()
{
	GLuint vertex = coreCompileShader(GL_VERTEX_SHADER, coreGLVertexShaderSource());
	GLuint fragment = coreCompileShader(GL_FRAGMENT_SHADER, coreGLFragmentShaderSource());
	if (vertex == 0 || fragment == 0)
	{
		if (vertex != 0) glDeleteShader(vertex);
		if (fragment != 0) glDeleteShader(fragment);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_TRUE)
		return program;

	GLint length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	std::vector<char> log(static_cast<std::size_t>(std::max(length, 1)), 0);
	glGetProgramInfoLog(program, length, nullptr, log.data());
	std::fprintf(stderr, "LegacyGL gl33 shader link failed: %s\n", log.data());
	glDeleteProgram(program);
	return 0;
}

static GLenum coreTopology(Topology topology)
{
	switch (topology)
	{
		case Topology::Points: return GL_POINTS;
		case Topology::Lines: return GL_LINES;
		default: return GL_TRIANGLES;
	}
}

static unsigned int coreAlphaFunction(unsigned int function)
{
	switch (function)
	{
		case GL_NEVER: return 0;
		case GL_LESS: return 1;
		case GL_EQUAL: return 2;
		case GL_LEQUAL: return 3;
		case GL_GREATER: return 4;
		case GL_NOTEQUAL: return 5;
		case GL_GEQUAL: return 6;
		default: return 7;
	}
}

static unsigned int coreFogMode(unsigned int mode, bool enabled)
{
	if (!enabled)
		return 0;
	if (mode == GL_LINEAR)
		return 1;
	if (mode == GL_EXP)
		return 2;
	return 3;
}

static unsigned int coreWrapMode(unsigned int mode)
{
	if (mode == GL_REPEAT)
		return 0;
	if (mode == GL_CLAMP)
		return 1;
	return 2;
}

static GLenum coreSamplerWrap(unsigned int mode)
{
	return mode == GL_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE;
}

static unsigned int coreColorMaterialFace(unsigned int face)
{
	if (face == GL_FRONT)
		return 1;
	if (face == GL_BACK)
		return 2;
	return 3;
}

static unsigned int coreColorMaterialMode(unsigned int mode)
{
	if (mode == GL_AMBIENT)
		return 0;
	if (mode == GL_DIFFUSE)
		return 1;
	if (mode == GL_AMBIENT_AND_DIFFUSE)
		return 2;
	if (mode == GL_SPECULAR)
		return 3;
	return 4;
}

static GLenum coreBaseMinFilter(unsigned int filter)
{
	switch (filter)
	{
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			return GL_NEAREST;
		default:
			return GL_LINEAR;
	}
}

static void coreCopyMaterial(CoreGPUMaterial &destination, const MaterialState &source)
{
	coreCopy4(destination.ambient, source.ambient);
	coreCopy4(destination.diffuse, source.diffuse);
	coreCopy4(destination.specular, source.specular);
	coreCopy4(destination.emission, source.emission);
	destination.shininess[0] = source.shininess;
	destination.shininess[1] = 0.0f;
	destination.shininess[2] = 0.0f;
	destination.shininess[3] = 0.0f;
}

static CoreGPUVertex coreGPUVertex(const Vertex &vertex)
{
	CoreGPUVertex result;
	result.position[0] = vertex.x; result.position[1] = vertex.y; result.position[2] = vertex.z;
	result.color[0] = vertex.r; result.color[1] = vertex.g; result.color[2] = vertex.b; result.color[3] = vertex.a;
	result.normal[0] = vertex.nx; result.normal[1] = vertex.ny; result.normal[2] = vertex.nz;
	result.texCoord[0] = vertex.s; result.texCoord[1] = vertex.t;
	result.drawSlot = 0;
	return result;
}

static CoreGPUFlatVertex coreGPUFlatVertex(const Vertex &vertex, const Vertex &flat)
{
	CoreGPUFlatVertex result;
	result.position[0] = vertex.x; result.position[1] = vertex.y; result.position[2] = vertex.z;
	result.color[0] = vertex.r; result.color[1] = vertex.g; result.color[2] = vertex.b; result.color[3] = vertex.a;
	result.normal[0] = vertex.nx; result.normal[1] = vertex.ny; result.normal[2] = vertex.nz;
	result.texCoord[0] = vertex.s; result.texCoord[1] = vertex.t;
	result.flatPosition[0] = flat.x; result.flatPosition[1] = flat.y; result.flatPosition[2] = flat.z;
	result.flatColor[0] = flat.r; result.flatColor[1] = flat.g; result.flatColor[2] = flat.b; result.flatColor[3] = flat.a;
	result.flatNormal[0] = flat.nx; result.flatNormal[1] = flat.ny; result.flatNormal[2] = flat.nz;
	result.drawSlot = 0;
	return result;
}

static CoreVertexLayout coreLayoutForBatch(const PrimitiveBatch &batch)
{
	return batch.sourceMode == GL_QUADS ? CoreVertexLayout::Expanded : CoreVertexLayout::Compact;
}

// Which optional attributes a draw supplies. The stream vertex array only has
// to be respecified when this changes, which is what keeps an attribute-aware
// layout from costing seven pointer calls on every transient draw.
static unsigned int coreAttributeMask(const Geometry &geometry)
{
	return (geometry.hasColor ? 1u : 0u) | (geometry.hasNormal ? 2u : 0u) |
		(geometry.hasTexCoord ? 4u : 0u);
}

// Redundant-state suppression, matching what the OpenGL 2.1 backend already
// does. Terrain replays thousands of display lists per frame that share one
// pipeline configuration, so applying it per draw cost roughly twenty GL calls
// each.
static bool coreSameEnables(const ResolvedEnableState &left, const ResolvedEnableState &right)
{
	static_assert(sizeof(ResolvedEnableState) == 16,
		"ResolvedEnableState is compared as a packed bool block");
	return std::memcmp(&left, &right, sizeof(ResolvedEnableState)) == 0;
}

static std::uint32_t coreFloatBits(float value)
{
	std::uint32_t result = 0;
	std::memcpy(&result, &value, sizeof(result));
	return result;
}

static bool coreSameBits(float left, float right)
{
	return coreFloatBits(left) == coreFloatBits(right);
}

static bool coreSamePipeline(const ResolvedPipelineState &left,
	const ResolvedPipelineState &right)
{
	for (int i = 0; i < 4; i++)
	{
		if (left.colorWrite[i] != right.colorWrite[i] || left.viewport[i] != right.viewport[i])
			return false;
	}
	return left.blendSource == right.blendSource &&
		left.blendDestination == right.blendDestination &&
		left.alphaFunction == right.alphaFunction &&
		coreSameBits(left.alphaReference, right.alphaReference) &&
		left.depthFunction == right.depthFunction &&
		left.depthWrite == right.depthWrite &&
		left.cullFaceMode == right.cullFaceMode &&
		left.frontFaceMode == right.frontFaceMode &&
		left.shadeModel == right.shadeModel &&
		left.logicOpcode == right.logicOpcode &&
		coreSameBits(left.lineWidth, right.lineWidth) &&
		coreSameBits(left.polygonOffsetFactor, right.polygonOffsetFactor) &&
		coreSameBits(left.polygonOffsetUnits, right.polygonOffsetUnits);
}

template <typename T>
static void coreAppendVertex(std::vector<unsigned char> &bytes, const T &vertex)
{
	const unsigned char *raw = reinterpret_cast<const unsigned char *>(&vertex);
	bytes.insert(bytes.end(), raw, raw + sizeof(T));
}

// Emits each canonical primitive with its legacy provoking vertex in the last
// slot, which under GL_LAST_VERTEX_CONVENTION is where the flat varying is
// sourced. Triangles are rotated cyclically, which preserves winding. Lines
// and points already arrive provoking-last from the canonicalizer, and a
// reversed line would rasterize differently, so those are checked rather than
// reordered. A legacy quad's first triangle cannot contain its provoking
// vertex at all; that source mode takes the expanded layout instead.
static CoreVertexData coreGPUVertices(const ResolvedDraw &command, int verticesPerPrimitive)
{
	const PrimitiveBatch &batch = *command.primitives;
	const std::vector<Vertex> &source = command.geometry->vertices;
	CoreVertexData data;
	data.layout = coreLayoutForBatch(batch);
	data.count = batch.primitives.size() * static_cast<std::size_t>(verticesPerPrimitive);
	data.bytes.reserve(data.count * static_cast<std::size_t>(coreVertexStride(data.layout)));

	for (const CanonicalPrimitive &primitive : batch.primitives)
	{
		if (data.layout == CoreVertexLayout::Expanded)
		{
			const Vertex &flat = source[static_cast<std::size_t>(primitive.provoking)];
			for (int i = 0; i < verticesPerPrimitive; i++)
			{
				coreAppendVertex(data.bytes, coreGPUFlatVertex(
					source[static_cast<std::size_t>(primitive.indices[i])], flat));
			}
			continue;
		}

		int order[3] = { 0, 1, 2 };
		if (verticesPerPrimitive == 3)
		{
			int provokingSlot = -1;
			for (int i = 0; i < 3; i++)
			{
				if (primitive.indices[i] == primitive.provoking)
					provokingSlot = i;
			}
			if (provokingSlot < 0)
			{
				throw std::runtime_error(
					"OpenGL 3.3 compact layout: provoking vertex outside its triangle");
			}
			order[0] = (provokingSlot + 1) % 3;
			order[1] = (provokingSlot + 2) % 3;
			order[2] = provokingSlot;
		}
		else if (primitive.indices[verticesPerPrimitive - 1] != primitive.provoking)
		{
			throw std::runtime_error(
				"OpenGL 3.3 compact layout: provoking vertex is not last in a line or point");
		}

		for (int i = 0; i < verticesPerPrimitive; i++)
		{
			coreAppendVertex(data.bytes, coreGPUVertex(
				source[static_cast<std::size_t>(primitive.indices[order[i]])]));
		}
	}
	return data;
}

class CoreGLSink : public Sink
{
public:
	void matrixMode(unsigned int) override {}
	void loadIdentity() override {}
	void pushMatrix() override {}
	void popMatrix() override {}
	void translatef(float, float, float) override {}
	void rotatef(float, float, float, float) override {}
	void scalef(float, float, float) override {}
	void scaled(double, double, double) override {}
	void ortho(double, double, double, double, double, double) override {}
	void frustum(double, double, double, double, double, double) override {}

	void enable(unsigned int) override {}
	void disable(unsigned int) override {}
	void blendFunc(unsigned int, unsigned int) override {}
	void alphaFunc(unsigned int, float) override {}
	void depthFunc(unsigned int) override {}
	void depthMask(unsigned char) override {}
	void colorMask(unsigned char, unsigned char, unsigned char, unsigned char) override {}
	void cullFace(unsigned int) override {}
	void shadeModel(unsigned int) override {}
	void logicOp(unsigned int) override {}
	void lineWidth(float) override {}
	void polygonOffset(float, float) override {}
	void viewport(int, int, int, int) override {}
	void pixelStorei(unsigned int, int) override {}

	void color4f(float, float, float, float) override {}
	void color3f(float, float, float) override {}
	void normal3f(float, float, float) override {}
	void normal3b(signed char, signed char, signed char) override {}

	void fogf(unsigned int, float) override {}
	void fogfv(unsigned int, const float *) override {}
	void fogi(unsigned int, int) override {}

	void lightfv(unsigned int, unsigned int, const float *) override {}
	void lightModelfv(unsigned int, const float *) override {}
	void colorMaterial(unsigned int, unsigned int) override {}

	void genTextures(int n, unsigned int *names) override
	{
		if (n <= 0 || names == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			names[i] = textureNames.allocate();
			if (names[i] == 0)
			{
				std::fprintf(stderr, "LegacyGL gl33: logical texture-name namespace exhausted\n");
				std::exit(EXIT_FAILURE);
			}
		}
	}

	void deleteTextures(int n, const unsigned int *names) override
	{
		if (n <= 0 || names == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			if (names[i] == 0)
				continue;
			textureNames.release(names[i]);
			auto found = textures.find(names[i]);
			if (found != textures.end())
			{
				if (found->second.sampler != 0)
					glDeleteSamplers(1, &found->second.sampler);
				if (found->second.handle != 0)
					glDeleteTextures(1, &found->second.handle);
				textures.erase(found);
				textureBindingValid = false;
			}
		}
	}

	void bindTexture(unsigned int, unsigned int name) override { textureNames.reserve(name); }
	void texParameteri(unsigned int, unsigned int, int) override {}
	void texImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *) override {}
	void texSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *) override {}

	void enableClientState(unsigned int) override {}
	void disableClientState(unsigned int) override {}
	void vertexPointer(int, unsigned int, int, const void *) override {}
	void texCoordPointer(int, unsigned int, int, const void *) override {}
	void colorPointer(int, unsigned int, int, const void *) override {}
	void normalPointer(unsigned int, int, const void *) override {}
	void drawArrays(unsigned int, int, int) override {}

	void begin(unsigned int) override {}
	void end() override {}
	void vertex3f(float, float, float) override {}
	void texCoord2f(float, float) override {}

	void genBuffersARB(int n, unsigned int *buffers) override
	{
		if (n <= 0 || buffers == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			buffers[i] = bufferNames.allocate();
			if (buffers[i] == 0)
			{
				std::fprintf(stderr, "LegacyGL gl33: logical buffer-name namespace exhausted\n");
				std::exit(EXIT_FAILURE);
			}
		}
	}
	void bindBufferARB(unsigned int, unsigned int name) override { bufferNames.reserve(name); }
	void bufferDataARB(unsigned int, std::ptrdiff_t, const void *, unsigned int) override {}

	unsigned int genLists(int range) override
	{
		if (range <= 0)
			return 0;
		const std::uint64_t count = static_cast<std::uint64_t>(range);
		const std::uint64_t maximumName = std::numeric_limits<unsigned int>::max();
		if (nextListName > maximumName || count > maximumName - nextListName + 1)
			return 0;
		const unsigned int base = static_cast<unsigned int>(nextListName);
		nextListName += count;
		return base;
	}
	void newList(unsigned int list, unsigned int) override
	{
		if (list != 0)
			nextListName = std::max(nextListName, static_cast<std::uint64_t>(list) + 1);
	}
	void endList() override {}
	void callList(unsigned int) override {}
	void callLists(int, unsigned int, const void *) override {}
	void deleteLists(unsigned int, int) override {}

	void clear(unsigned int) override {}
	void clearColor(float, float, float, float) override {}
	void clearDepth(double) override {}
	void readPixels(int, int, int, int, unsigned int, unsigned int, void *) override {}
	void finish() override { glFinish(); }

	bool wantsCanonicalGeometry() const override { return true; }
	bool isCanonicalGeometryResident(std::uint64_t residencyId) const override
	{
		return residencyId != 0 && residentGeometry.find(residencyId) != residentGeometry.end();
	}
	bool queryResidentStats(legacygl::Sink::ResidentStats &out) const override
	{
		out.logicalBytes = residentGeometryBytes;
		out.entries = residentGeometry.size();
		out.pages = 0;
		out.pageCapacityBytes = 0;
		for (const CoreResidentPage &page : residentPages)
		{
			if (page.buffer == 0)
				continue;
			out.pages++;
			out.pageCapacityBytes += static_cast<std::uint64_t>(page.capacity);
		}
		out.batchedDraws = batchedDraws;
		out.multidraws = batchedDrawCalls;
		out.batchBlockOverflows = batchBlockOverflows;
		return true;
	}

	void releaseCanonicalGeometry(std::uint64_t residencyId) override
	{
		if (residencyId == 0)
			return;
		flushBatch();
		auto entry = residentGeometry.find(residencyId);
		if (entry == residentGeometry.end())
			return;
		residentGeometryBytes -= static_cast<std::uint64_t>(entry->second.byteSize);
		residentGeometryReleases++;
		freeResidentRange(entry->second);
		residentGeometry.erase(entry);
	}

	// Returns a range to its page and merges it with any neighbour. A completely
	// empty page is deleted and its stable vector slot is reused later.
	void freeResidentRange(const CoreResidentGeometryEntry &entry)
	{
		if (entry.page >= residentPages.size())
			return;
		CoreResidentPage &page = residentPages[entry.page];
		if (page.buffer == 0 || page.liveEntries == 0)
			return;
		page.liveEntries--;
		page.freeSlots.push_back(entry.slot);
		if (page.liveEntries == 0)
		{
			if (boundVertexArray == page.vertexArray)
			{
				glBindVertexArray(0);
				boundVertexArray = 0;
			}
			glDeleteVertexArrays(1, &page.vertexArray);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDeleteBuffers(1, &page.buffer);
			page = CoreResidentPage();
			return;
		}

		const CoreResidentFreeRange released = { entry.byteOffset, entry.byteSize };
		std::size_t insertAt = page.freeRanges.size();
		for (std::size_t i = 0; i < page.freeRanges.size(); i++)
		{
			if (page.freeRanges[i].offset > released.offset)
			{
				insertAt = i;
				break;
			}
		}
		page.freeRanges.insert(page.freeRanges.begin() +
			static_cast<std::ptrdiff_t>(insertAt), released);
		for (std::size_t i = 0; i + 1 < page.freeRanges.size();)
		{
			CoreResidentFreeRange &current = page.freeRanges[i];
			const CoreResidentFreeRange &next = page.freeRanges[i + 1];
			if (current.offset + current.size == next.offset)
			{
				current.size += next.size;
				page.freeRanges.erase(page.freeRanges.begin() +
					static_cast<std::ptrdiff_t>(i) + 1);
				continue;
			}
			i++;
		}
	}

	// Sub-allocates from a page whose vertex array already describes this
	// attribute set and vertex layout, uploading into the shared buffer.
	CoreResidentGeometryEntry allocateResidentGeometry(const ResolvedDraw &command,
		const Geometry &geometry, CoreVertexData &vertices)
	{
		const GLsizeiptr size = vertices.byteSize();
		const unsigned int attributeMask = coreAttributeMask(geometry);
		const CoreVertexLayout layout = vertices.layout;

		std::size_t pageIndex = residentPages.size();
		GLsizeiptr offset = 0;
		for (std::size_t i = 0; i < residentPages.size(); i++)
		{
			CoreResidentPage &page = residentPages[i];
			if (page.buffer == 0 || page.attributeMask != attributeMask || page.layout != layout)
				continue;
			for (std::size_t range = 0; range < page.freeRanges.size(); range++)
			{
				if (page.freeRanges[range].size < size)
					continue;
				offset = page.freeRanges[range].offset;
				page.freeRanges[range].offset += size;
				page.freeRanges[range].size -= size;
				if (page.freeRanges[range].size == 0)
				{
					page.freeRanges.erase(page.freeRanges.begin() +
						static_cast<std::ptrdiff_t>(range));
				}
				pageIndex = i;
				break;
			}
			if (pageIndex != residentPages.size())
				break;
		}

		if (pageIndex == residentPages.size())
		{
			CoreResidentPage page;
			page.attributeMask = attributeMask;
			page.layout = layout;
			page.capacity = std::max<GLsizeiptr>(CORE_RESIDENT_PAGE_BYTES, size);
			page.liveEntries = 1;
			glGenVertexArrays(1, &page.vertexArray);
			glGenBuffers(1, &page.buffer);
			configureVertexArray(page.vertexArray, page.buffer, geometry, layout);
			glBufferData(GL_ARRAY_BUFFER, page.capacity, nullptr, GL_STATIC_DRAW);
			if (page.capacity > size)
				page.freeRanges.push_back({ size, page.capacity - size });
			offset = 0;

			for (std::size_t i = 0; i < residentPages.size(); i++)
			{
				if (residentPages[i].buffer != 0)
					continue;
				residentPages[i] = std::move(page);
				pageIndex = i;
				break;
			}
			if (pageIndex == residentPages.size())
				residentPages.push_back(std::move(page));
		}
		else
		{
			residentPages[pageIndex].liveEntries++;
		}

		CoreResidentPage &page = residentPages[pageIndex];

		// Dense per-page slot, stamped into every vertex so a batched draw can
		// fetch its own matrices. The stride is the same for both layouts'
		// trailing field, so one loop serves both.
		std::uint32_t slot;
		if (!page.freeSlots.empty())
		{
			slot = page.freeSlots.back();
			page.freeSlots.pop_back();
		}
		else
		{
			slot = page.nextSlot++;
		}
		{
			const std::size_t stride = static_cast<std::size_t>(coreVertexStride(layout));
			const std::size_t slotOffset = layout == CoreVertexLayout::Expanded ?
				offsetof(CoreGPUFlatVertex, drawSlot) : offsetof(CoreGPUVertex, drawSlot);
			for (std::size_t i = 0; i < vertices.count; i++)
			{
				const std::uint32_t stamped = slot & (CORE_BATCH_SLOTS - 1);
				std::memcpy(vertices.bytes.data() + i * stride + slotOffset, &stamped, sizeof(stamped));
			}
		}
		// Staging takes at most half the region so a rebuild burst cannot push
		// the frame's own transient draws into the drain path.
		GLsizeiptr staged = 0;
		if (streamMapped != nullptr && streamCursor + size <= CORE_STREAM_REGION_BYTES / 2 &&
			tryReserveStreamBytes(size, 4, staged))
		{
			// Stage through the fence-ringed persistent stream and let the GPU
			// copy into the page. glBufferSubData into a page that in-flight
			// draws are reading made nouveau (Switch) wait for those draws:
			// measured ~11 ms per rebuilt section, ~40 ms of an 83 ms frame at
			// idle. The copy is ordered in the command stream after the reads
			// of whatever previously occupied the range, so freed ranges can be
			// reused without a fence. A rebuild burst that outgrows the region
			// (world load) takes the direct path rather than draining the GPU.
			std::memcpy(streamMapped + staged, vertices.bytes.data(),
				static_cast<std::size_t>(size));
			glBindBuffer(GL_COPY_READ_BUFFER, vertexBuffer);
			glBindBuffer(GL_COPY_WRITE_BUFFER, page.buffer);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, staged, offset, size);
			residentStagedUploads++;
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, page.buffer);
			glBufferSubData(GL_ARRAY_BUFFER, offset, size, vertices.bytes.data());
			residentDirectUploads++;
		}

		CoreResidentGeometryEntry entry;
		entry.page = pageIndex;
		entry.byteOffset = offset;
		entry.byteSize = size;
		entry.firstVertex =
			static_cast<GLint>(offset / static_cast<GLsizeiptr>(coreVertexStride(layout)));
		entry.topology = command.primitives->topology;
		entry.vertexCount = vertices.count;
		entry.hasColor = geometry.hasColor;
		entry.hasNormal = geometry.hasNormal;
		entry.hasTexCoord = geometry.hasTexCoord;
		entry.slot = slot;
		return entry;
	}

	void releaseAllCanonicalGeometry()
	{
		batchCount = 0;
		for (CoreResidentPage &page : residentPages)
		{
			if (page.vertexArray != 0)
				glDeleteVertexArrays(1, &page.vertexArray);
			if (page.buffer != 0)
				glDeleteBuffers(1, &page.buffer);
		}
		residentPages.clear();
		residentGeometry.clear();
		residentGeometryBytes = 0;
		boundVertexArray = 0;
	}
	void shutdown()
	{
		flushBatch();
		std::size_t residentPageCount = 0;
		std::uint64_t residentPageBytes = 0;
		for (const CoreResidentPage &page : residentPages)
		{
			if (page.buffer == 0)
				continue;
			residentPageCount++;
			residentPageBytes += static_cast<std::uint64_t>(page.capacity);
		}
		std::fprintf(stdout,
			"gl33: shutdown, resident cache hits=%llu, misses=%llu,"
			" resident bytes=%llu, entries=%zu, pages=%zu, page bytes=%llu,"
			" explicit releases=%llu\n",
			static_cast<unsigned long long>(residentCacheHits),
			static_cast<unsigned long long>(residentCacheMisses),
			static_cast<unsigned long long>(residentGeometryBytes),
			residentGeometry.size(), residentPageCount,
			static_cast<unsigned long long>(residentPageBytes),
			static_cast<unsigned long long>(residentGeometryReleases));
		const char *modeName =
			uniformMode == CoreUniformMode::Persistent ? "persistent" : "legacy";
		std::fprintf(stdout,
			"gl33: uniform mode=%s stride=%zu slots=%zu peak_slots=%zu"
			" overflows=%llu fence_waits=%llu\n",
			modeName, uniformStride, uniformSlots, uniformPeakSlots,
			static_cast<unsigned long long>(uniformOverflows),
			static_cast<unsigned long long>(uniformFenceWaits));
		std::fprintf(stdout,
			"gl33: vertex stream=%s region=%lld peak_bytes=%lld overflows=%llu"
			" resident staged=%llu direct=%llu\n",
			streamMapped != nullptr ? "persistent" : "glBufferData",
			static_cast<long long>(CORE_STREAM_REGION_BYTES),
			static_cast<long long>(streamPeakBytes),
			static_cast<unsigned long long>(streamOverflows),
			static_cast<unsigned long long>(residentStagedUploads),
			static_cast<unsigned long long>(residentDirectUploads));
		std::fprintf(stdout,
			"gl33: resident batches: draws=%llu multidraws=%llu breaks topo=%llu state=%llu other=%llu block_overflows=%llu\n",
			static_cast<unsigned long long>(batchedDraws),
			static_cast<unsigned long long>(batchedDrawCalls),
			static_cast<unsigned long long>(batchBreakTopology),
			static_cast<unsigned long long>(batchBreakState),
			static_cast<unsigned long long>(batchBreakOther),
			static_cast<unsigned long long>(batchBlockOverflows));
		batchedDrawCalls = 0;
		batchedDraws = 0;
		batchBreakTopology = batchBreakState = batchBreakOther = 0;
		batchBlockOverflows = 0;
		releaseAllCanonicalGeometry();
		residentCacheHits = 0;
		residentCacheMisses = 0;
		residentGeometryReleases = 0;
	}

	// Two ways to get a per-draw uniform block to the GPU, selectable with
	// A126_GL33_UNIFORM=persistent|legacy:
	//
	//   persistent  one immutable coherent mapping, a disjoint slot per draw,
	//               one fence per frame region (default when available)
	//   legacy      a single overwritten offset, used when immutable storage
	//               or persistent mapping is unavailable
	//
	// A third variant, a mutable buffer orphaned once per frame with a disjoint
	// glBufferSubData per draw, measured 23.2ms against 9.4ms for legacy on an
	// RTX 5070 and was removed. On the same machine persistent is only about 4%
	// ahead of legacy. That is a proprietary-driver result: on the Switch's
	// Mesa/nouveau the legacy glBufferSubData per draw measured 4.4us, 29% of
	// the whole draw path, so the transport matters there.

	// A126_GL33_UNIFORM selects the transport; persistent is the default when
	// immutable storage and mapping are available. GL_ARB_buffer_storage is the
	// same functionality on a 4.3 context, so the extension counts as well as
	// the 4.4 core version; the capability report logs which one was found.
	static CoreUniformMode selectUniformMode()
	{
		const char *requested = std::getenv("A126_GL33_UNIFORM");
		const bool canPersist =
			(GLAD_GL_VERSION_4_4 != 0 || GLAD_GL_ARB_buffer_storage != 0) &&
			glad_glBufferStorage != nullptr && glad_glMapBufferRange != nullptr &&
			glad_glFenceSync != nullptr && glad_glClientWaitSync != nullptr;
		if (requested != nullptr)
		{
			if (std::strcmp(requested, "legacy") == 0)
				return CoreUniformMode::Legacy;
			if (std::strcmp(requested, "persistent") == 0 && canPersist)
				return CoreUniformMode::Persistent;
		}
		return canPersist ? CoreUniformMode::Persistent : CoreUniformMode::Legacy;
	}

	void beginFrame()
	{
		if (!initialized)
			return;
		flushBatch();
		uniformPeakSlots = std::max(uniformPeakSlots, uniformCursor);
		if (uniformMode == CoreUniformMode::Legacy)
			return;

		// Persistent: retire the region just recorded, then wait for the region
		// about to be reused. One fence per region, never per draw.
		if (uniformFences[uniformRegion] != nullptr)
			glDeleteSync(uniformFences[uniformRegion]);
		uniformFences[uniformRegion] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		uniformRegion = (uniformRegion + 1) % CORE_UNIFORM_REGIONS;
		if (uniformFences[uniformRegion] != nullptr)
		{
			const GLenum waited = glClientWaitSync(uniformFences[uniformRegion],
				GL_SYNC_FLUSH_COMMANDS_BIT, 0);
			if (waited == GL_TIMEOUT_EXPIRED)
			{
				uniformFenceWaits++;
				glClientWaitSync(uniformFences[uniformRegion],
					GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
			}
			glDeleteSync(uniformFences[uniformRegion]);
			uniformFences[uniformRegion] = nullptr;
		}
		uniformCursor = 0;
		batchBlockCursor = 0;
		streamPeakBytes = std::max(streamPeakBytes, streamCursor);
		streamCursor = 0;
	}

	// Returns the byte offset of the slot this draw owns.
	GLintptr reserveUniformSlot()
	{
		if (uniformMode == CoreUniformMode::Legacy)
			return 0;
		if (uniformCursor >= uniformSlots)
		{
			// A frame outgrew its region. Draining is heavy but correct, and
			// the region is enlarged for subsequent frames so it stops
			// happening rather than being hidden.
			uniformPeakSlots = std::max(uniformPeakSlots, uniformCursor);
			uniformOverflows++;
			glFinish();
			uniformCursor = 0;
		}
		const std::size_t regionBase = uniformMode == CoreUniformMode::Persistent ?
			uniformRegion * uniformSlots * uniformStride : 0;
		const GLintptr offset =
			static_cast<GLintptr>(regionBase + uniformCursor * uniformStride);
		uniformCursor++;
		return offset;
	}

	// One 16 KiB matrix block per page run. The arena rotates with the state
	// regions, so the same fence protects both. Overflow drains like the
	// state ring does.
	GLintptr reserveBatchBlock()
	{
		if (uniformMode == CoreUniformMode::Legacy)
			return 0;
		if (batchBlockCursor >= CORE_BATCH_BLOCKS)
		{
			batchBlockOverflows++;
			glFinish();
			batchBlockCursor = 0;
		}
		const GLintptr offset = batchArenaBase +
			static_cast<GLintptr>((uniformRegion * CORE_BATCH_BLOCKS + batchBlockCursor) * sizeof(CoreBatchBlock));
		batchBlockCursor++;
		return offset;
	}

	// Reserves size bytes in the current persistent stream region, aligned to
	// align, and returns the absolute byte offset within vertexBuffer. False
	// when the region has no room left this frame.
	bool tryReserveStreamBytes(GLsizeiptr size, GLsizeiptr align, GLsizeiptr &absolute)
	{
		const GLsizeiptr regionBase = static_cast<GLsizeiptr>(uniformRegion) *
			CORE_STREAM_REGION_BYTES;
		absolute = (regionBase + streamCursor + align - 1) / align * align;
		if (absolute + size > regionBase + CORE_STREAM_REGION_BYTES)
			return false;
		streamCursor = absolute + size - regionBase;
		return true;
	}

	// The transient-draw form: a frame that outgrows its region drains the
	// GPU and reuses it. Counted so the shutdown line shows it rather than
	// hiding it.
	GLsizeiptr reserveStreamBytes(GLsizeiptr size, GLsizeiptr align)
	{
		GLsizeiptr absolute = 0;
		if (tryReserveStreamBytes(size, align, absolute))
			return absolute;
		streamOverflows++;
		glFinish();
		streamCursor = 0;
		tryReserveStreamBytes(size, align, absolute);
		return absolute;
	}

	void writeUniformSlot(GLintptr offset, const CoreGPUState &state)
	{
		if (uniformMode == CoreUniformMode::Persistent)
		{
			std::memcpy(uniformMapped + offset, &state, sizeof(state));
			return;
		}
		glBufferSubData(GL_UNIFORM_BUFFER, offset,
			static_cast<GLsizeiptr>(sizeof(state)), &state);
	}
	// Whether two resolved draws can share one glMultiDrawArrays: everything
	// the uniform block and pipeline carry must match except the two matrices
	// the batch block supplies per slot.
	static bool coreBatchCompatible(const CoreGPUState &left, const CoreGPUState &right)
	{
		const std::size_t skipBegin = offsetof(CoreGPUState, modelView);
		const std::size_t skipEnd = skipBegin + sizeof(left.modelView);
		const std::size_t normalBegin = offsetof(CoreGPUState, normal);
		const std::size_t normalEnd = normalBegin + sizeof(left.normal);
		const unsigned char *l = reinterpret_cast<const unsigned char *>(&left);
		const unsigned char *r = reinterpret_cast<const unsigned char *>(&right);
		// Layout: modelView, projection, texture, normal, then everything else.
		// Compare the projection+texture span and the tail separately.
		return std::memcmp(l + skipEnd, r + skipEnd, normalBegin - skipEnd) == 0 &&
			std::memcmp(l + normalEnd, r + normalEnd, sizeof(CoreGPUState) - normalEnd) == 0;
	}

	// Resident hit with a page slot inside the batch block: append to the
	// pending batch if it is compatible, else flush and start a new one.
	bool tryBatchResidentDraw(const ResolvedDraw &command, std::size_t vertexCount)
	{
		auto found = residentGeometry.find(command.geometryResidencyId);
		if (found == residentGeometry.end())
			return false;
		const CoreResidentGeometryEntry &entry = found->second;
		if (entry.topology != command.primitives->topology || entry.vertexCount != vertexCount)
			throw std::runtime_error("OpenGL 3.3 resident geometry identity changed while cached");
		residentCacheHits++;

		CoreGPUState state = {};
		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::StatePack);
		fillGPUState(command, state);
		}

		const bool joins = batchCount > 0 &&
			batchTopology == entry.topology && coreBatchCompatible(batchState, state) &&
			coreSameEnables(batchCommand.enables, command.enables) &&
			coreSamePipeline(batchCommand.pipeline, command.pipeline) &&
			batchCommand.texture.name == command.texture.name &&
			std::memcmp(&batchCommand.currentAttributes, &command.currentAttributes,
				sizeof(Vertex)) == 0;
		if (!joins && batchCount > 0)
		{
			if (batchTopology != entry.topology) batchBreakTopology++;
			else if (!coreBatchCompatible(batchState, state)) batchBreakState++;
			else batchBreakOther++;
		}
		if (!joins)
		{
			flushBatch();
			batchTopology = entry.topology;
			batchState = state;
			batchCommand = command;
		}
		CoreBatchedDraw &draw = batchDraws[batchCount++];
		draw.page = entry.page;
		draw.slot = entry.slot;
		draw.first = entry.firstVertex;
		draw.count = static_cast<GLsizei>(vertexCount);
		std::memcpy(draw.modelView.m, command.modelView.m, sizeof(Mat4));
		std::memcpy(draw.normal.m, command.normal.m, sizeof(Mat4));
		if (batchCount == CORE_BATCH_SLOTS)
			flushBatch();
		return true;
	}

	// Issues the pending batch. One state upload with the batch flag; then,
	// per run of draws sharing a page, one 16 KiB matrix block indexed by the
	// entries' page slots and one glMultiDrawArrays. Slots not in the run keep
	// stale matrices, which no vertex reads. The matrix block lives in the
	// same fence-ringed persistent buffer as the state blocks, one region
	// ahead of the state slots.
	void flushBatch()
	{
		if (batchCount == 0)
			return;
		const std::size_t count = batchCount;
		batchCount = 0;
		batchedDraws += count;

		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::Bind);
		applyPipeline(batchCommand);
		bindTexture(batchCommand, batchState);
		glUseProgram(program);
		}
		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::StateUpload);
		batchState.flags0[3] |= 4u;
		const GLintptr uniformOffset = reserveUniformSlot();
		writeUniformSlot(uniformOffset, batchState);
		if (uniformMode != CoreUniformMode::Legacy)
		{
			glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer, uniformOffset,
				static_cast<GLsizeiptr>(sizeof(CoreGPUState)));
		}
		}
		applyCurrentAttributes(batchCommand);

		std::size_t runStart = 0;
		while (runStart < count)
		{
			const std::size_t page = batchDraws[runStart].page;
			std::size_t runEnd = runStart + 1;
			// A run shares one matrix block, indexed by the low 7 bits of the
			// page slot; a run must therefore not contain two entries that
			// alias. Split there.
			std::uint64_t used[2] = { 0, 0 };
			{
				const std::uint32_t b = batchDraws[runStart].slot & (CORE_BATCH_SLOTS - 1);
				used[b >> 6] |= 1ull << (b & 63);
			}
			while (runEnd < count && batchDraws[runEnd].page == page)
			{
				const std::uint32_t b = batchDraws[runEnd].slot & (CORE_BATCH_SLOTS - 1);
				if (used[b >> 6] & (1ull << (b & 63)))
					break;
				used[b >> 6] |= 1ull << (b & 63);
				runEnd++;
			}
			const std::size_t runCount = runEnd - runStart;
			batchedDrawCalls++;
			batchPageRuns++;

			{
			legacygl::PhaseScope phase(legacygl::DrawPhase::StateUpload);
			const GLintptr blockOffset = reserveBatchBlock();
			// Only the rows this run indexes are written; the shader never
			// reads the others, so stale rows in the mapping are harmless.
			unsigned char *block = uniformMode == CoreUniformMode::Persistent ?
				uniformMapped + blockOffset : reinterpret_cast<unsigned char *>(&batchBlock);
			for (std::size_t i = 0; i < runCount; i++)
			{
				const CoreBatchedDraw &draw = batchDraws[runStart + i];
				batchFirst[i] = draw.first;
				batchCounts[i] = draw.count;
				const std::size_t row = draw.slot & (CORE_BATCH_SLOTS - 1);
				std::memcpy(block + offsetof(CoreBatchBlock, modelView) + row * sizeof(Mat4),
					&draw.modelView, sizeof(Mat4));
				std::memcpy(block + offsetof(CoreBatchBlock, normal) + row * sizeof(Mat4),
					&draw.normal, sizeof(Mat4));
			}
			if (uniformMode == CoreUniformMode::Persistent)
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniformBuffer, blockOffset,
					static_cast<GLsizeiptr>(sizeof(batchBlock)));
			}
			else
			{
				glBindBuffer(GL_UNIFORM_BUFFER, batchUniformBuffer);
				glBufferData(GL_UNIFORM_BUFFER, sizeof(batchBlock), &batchBlock, GL_STREAM_DRAW);
				glBindBufferBase(GL_UNIFORM_BUFFER, 1, batchUniformBuffer);
				glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
			}
			}
			{
			legacygl::PhaseScope phase(legacygl::DrawPhase::Bind);
			const GLuint pageArray = residentPages[page].vertexArray;
			if (boundVertexArray != pageArray)
			{
				glBindVertexArray(pageArray);
				boundVertexArray = pageArray;
			}
			}
			{
			legacygl::PhaseScope phase(legacygl::DrawPhase::Draw);
			glMultiDrawArrays(coreTopology(batchTopology), batchFirst, batchCounts,
				static_cast<GLsizei>(runCount));
			}
			runStart = runEnd;
		}
	}

	void resolvedDraw(const ResolvedDraw &command) override
	{
		initialize();
		if (command.geometry == nullptr || command.primitives == nullptr)
			return;

		const Geometry &geometry = *command.geometry;
		const int verticesPerPrimitive = command.primitives->topology == Topology::Points ? 1 :
			(command.primitives->topology == Topology::Lines ? 2 : 3);
		const std::size_t vertexCount = command.primitives->primitives.size() *
			static_cast<std::size_t>(verticesPerPrimitive);
		if (vertexCount == 0)
			return;

		// A resident hit whose state matches the pending batch joins it and
		// returns; anything else is a batch boundary. Ordering is preserved
		// because the batch is always flushed before the draw that broke it.
		if (command.geometryResidencyId != 0 && tryBatchResidentDraw(command, vertexCount))
			return;
		flushBatch();

		GLuint drawVertexArray = vertexArray;
		GLint drawFirstVertex = 0;
		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::Geometry);
		if (command.geometryResidencyId == 0)
		{
			legacygl::PhaseScope upload(legacygl::DrawPhase::GeometryUpload);
			if (geometry.vertices.empty())
				throw std::runtime_error("OpenGL 3.3 transient geometry has no CPU source");
			const CoreVertexData vertices = coreGPUVertices(command, verticesPerPrimitive);
			const unsigned int attributeMask = coreAttributeMask(geometry);
			const GLsizeiptr stride = coreVertexStride(vertices.layout);
			GLuint sourceBuffer = vertexBuffer;

			if (streamMapped != nullptr && vertices.byteSize() <= CORE_STREAM_REGION_BYTES)
			{
				// First-vertex addressing needs the absolute offset to be a
				// multiple of the stride, so align it as such within the region.
				const GLsizeiptr absolute = reserveStreamBytes(vertices.byteSize(), stride);
				std::memcpy(streamMapped + absolute, vertices.bytes.data(),
					static_cast<std::size_t>(vertices.byteSize()));
				drawFirstVertex = static_cast<GLint>(absolute / stride);
			}
			else if (streamMapped != nullptr)
			{
				// Larger than a region: a mutable buffer of its own. The vertex
				// array must be re-pointed at it and back afterwards.
				if (streamOverflowBuffer == 0)
					glGenBuffers(1, &streamOverflowBuffer);
				sourceBuffer = streamOverflowBuffer;
				streamLayoutValid = false;
			}

			if (!streamLayoutValid || streamAttributeMask != attributeMask ||
				streamVertexLayout != vertices.layout)
			{
				configureVertexArray(vertexArray, sourceBuffer, geometry, vertices.layout);
				streamAttributeMask = attributeMask;
				streamVertexLayout = vertices.layout;
				streamLayoutValid = sourceBuffer == vertexBuffer;
			}
			else
			{
				glBindVertexArray(vertexArray);
				boundVertexArray = vertexArray;
			}

			if (sourceBuffer != vertexBuffer || streamMapped == nullptr)
			{
				glBindBuffer(GL_ARRAY_BUFFER, sourceBuffer);
				glBufferData(GL_ARRAY_BUFFER, vertices.byteSize(), vertices.bytes.data(),
					GL_STREAM_DRAW);
			}
		}
		else
		{
			auto found = residentGeometry.find(command.geometryResidencyId);
			if (found != residentGeometry.end())
			{
				residentCacheHits++;
				if (found->second.topology != command.primitives->topology ||
					found->second.vertexCount != vertexCount ||
					found->second.hasColor != geometry.hasColor ||
					found->second.hasNormal != geometry.hasNormal ||
					found->second.hasTexCoord != geometry.hasTexCoord)
				{
					throw std::runtime_error(
						"OpenGL 3.3 resident geometry identity changed while cached");
				}
			}
			else
			{
				legacygl::PhaseScope upload(legacygl::DrawPhase::GeometryResidentUpload);
				if (geometry.vertices.empty())
				{
					throw std::runtime_error(
						"OpenGL 3.3 resident geometry source was released before upload");
				}
				residentCacheMisses++;
				CoreVertexData vertices = coreGPUVertices(command, verticesPerPrimitive);
				found = residentGeometry.emplace(command.geometryResidencyId,
					allocateResidentGeometry(command, geometry, vertices)).first;
				residentGeometryBytes += static_cast<std::uint64_t>(found->second.byteSize);
			}
			drawVertexArray = residentPages[found->second.page].vertexArray;
			drawFirstVertex = found->second.firstVertex;
		}
		}

		CoreGPUState state = {};
		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::StatePack);
		fillGPUState(command, state);
		}
		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::Bind);
		applyPipeline(command);
		bindTexture(command, state);
		glUseProgram(program);
		}

		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::StateUpload);
		const GLintptr uniformOffset = reserveUniformSlot();
		writeUniformSlot(uniformOffset, state);
		if (uniformMode != CoreUniformMode::Legacy)
		{
			// glBindBufferRange also refreshes the generic GL_UNIFORM_BUFFER
			// binding, so no separate glBindBuffer is needed for the next write.
			glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer, uniformOffset,
				static_cast<GLsizeiptr>(sizeof(state)));
		}
		}

		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::Bind);
		if (boundVertexArray != drawVertexArray)
		{
			glBindVertexArray(drawVertexArray);
			boundVertexArray = drawVertexArray;
		}
		applyCurrentAttributes(command);
		}
		{
		legacygl::PhaseScope phase(legacygl::DrawPhase::Draw);
		glDrawArrays(coreTopology(command.primitives->topology), drawFirstVertex,
			static_cast<GLsizei>(vertexCount));
		}
	}

	void resolvedClear(const ResolvedClear &command) override
	{
		initialize();
		flushBatch();
		glColorMask(command.colorWrite[0], command.colorWrite[1], command.colorWrite[2], command.colorWrite[3]);
		glDepthMask(command.depthWrite);
		if (command.scissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
		if (command.dither) glEnable(GL_DITHER); else glDisable(GL_DITHER);
		glClearColor(command.color[0], command.color[1], command.color[2], command.color[3]);
		glClearDepth(command.depth);
		glClear(command.mask);
		// The clear path writes the same masks and enables that applyPipeline
		// caches, so the next draw has to reapply them.
		pipelineStateValid = false;
	}

	void resolvedTextureUpload(const ResolvedTextureUpload &command) override
	{
		initialize();
		flushBatch();
		CoreTexture &texture = ensureTexture(command.texture);
		if (command.level < 0 || command.level >= CORE_TEXTURE_LEVELS)
			return;

		CoreTextureLevel &level = texture.levels[command.level];
		if (!command.subImage)
		{
			level.width = command.width;
			level.height = command.height;
			level.defined = true;
			level.rgba.assign(static_cast<std::size_t>(command.width) * static_cast<std::size_t>(command.height) * 4, 0);
		}

		const PixelTransferFormat *transfer =
			unsignedBytePixelTransferFormat(command.sourceFormat);
		PixelStorageFormat storage;
		if (command.sourceType != GL_UNSIGNED_BYTE || transfer == nullptr ||
			!pixelStorageFormat(command.internalFormat, storage) ||
			storage.physical != PhysicalPixelFormat::RGBA8)
		{
			throw std::runtime_error("OpenGL 3.3 texture upload received an unsupported pixel format");
		}

		if (command.pixels != nullptr && command.width > 0 && command.height > 0)
		{
			const int components = transfer->components;
			const std::size_t sourceRow = coreAlignedRowSize(
				static_cast<std::size_t>(command.width) * static_cast<std::size_t>(components), command.unpackAlignment);
			const unsigned char *source = static_cast<const unsigned char *>(command.pixels);
			for (int y = 0; y < command.height; y++)
			{
				for (int x = 0; x < command.width; x++)
				{
					unsigned char rgba[4];
					if (!decodeUnsignedBytePixel(source + static_cast<std::size_t>(y) * sourceRow +
						static_cast<std::size_t>(x) * static_cast<std::size_t>(components),
						command.sourceFormat, rgba) ||
						!applyIntendedPixelFormat(storage.intended, rgba))
					{
						throw std::runtime_error("OpenGL 3.3 texture upload conversion failed");
					}
					const std::size_t destination = (static_cast<std::size_t>(command.y + y) *
						static_cast<std::size_t>(level.width) + static_cast<std::size_t>(command.x + x)) * 4;
					std::memcpy(level.rgba.data() + destination, rgba, 4);
				}
			}
		}
		texture.derivedDirty = true;
	}

	void resolvedReadback(const ResolvedReadback &command) override
	{
		initialize();
		flushBatch();
		if (command.width == 0 || command.height == 0)
			return;

		const PixelTransferFormat *transfer =
			unsignedBytePixelTransferFormat(command.format);
		if (command.type != GL_UNSIGNED_BYTE || transfer == nullptr)
			throw std::runtime_error("OpenGL 3.3 readback received an unsupported pixel format");

		std::vector<unsigned char> rgba(static_cast<std::size_t>(command.width) *
			static_cast<std::size_t>(command.height) * 4);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(command.x, command.y, command.width, command.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

		const int components = transfer->components;
		const std::size_t destinationRow = coreAlignedRowSize(
			static_cast<std::size_t>(command.width) * static_cast<std::size_t>(components), command.packAlignment);
		unsigned char *destination = static_cast<unsigned char *>(command.pixels);
		for (int y = 0; y < command.height; y++)
		{
			for (int x = 0; x < command.width; x++)
			{
				const unsigned char *source = rgba.data() +
					(static_cast<std::size_t>(y) * static_cast<std::size_t>(command.width) + static_cast<std::size_t>(x)) * 4;
				if (!encodeUnsignedBytePixel(source, command.format,
					destination + static_cast<std::size_t>(y) * destinationRow +
						static_cast<std::size_t>(x) * static_cast<std::size_t>(components)))
				{
					throw std::runtime_error("OpenGL 3.3 readback conversion failed");
				}
			}
		}
	}

private:
	// The shader's flat inputs (locations 4/5/6) alias the smooth fields in the
	// compact layout, because the provoking vertex is last and the flat varying
	// takes its value from it; only the expanded layout has separate fields.
	void configureVertexArray(GLuint array, GLuint buffer, const Geometry &geometry,
		CoreVertexLayout layout)
	{
		const bool expanded = layout == CoreVertexLayout::Expanded;
		const GLsizei stride = coreVertexStride(layout);
		const std::size_t positionOffset = expanded ?
			offsetof(CoreGPUFlatVertex, position) : offsetof(CoreGPUVertex, position);
		const std::size_t colorOffset = expanded ?
			offsetof(CoreGPUFlatVertex, color) : offsetof(CoreGPUVertex, color);
		const std::size_t normalOffset = expanded ?
			offsetof(CoreGPUFlatVertex, normal) : offsetof(CoreGPUVertex, normal);
		const std::size_t texCoordOffset = expanded ?
			offsetof(CoreGPUFlatVertex, texCoord) : offsetof(CoreGPUVertex, texCoord);
		const std::size_t flatPositionOffset = expanded ?
			offsetof(CoreGPUFlatVertex, flatPosition) : positionOffset;
		const std::size_t flatColorOffset = expanded ?
			offsetof(CoreGPUFlatVertex, flatColor) : colorOffset;
		const std::size_t flatNormalOffset = expanded ?
			offsetof(CoreGPUFlatVertex, flatNormal) : normalOffset;

		glBindVertexArray(array);
		boundVertexArray = array;
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
			reinterpret_cast<const void *>(positionOffset));

		if (geometry.hasColor)
		{
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
				reinterpret_cast<const void *>(colorOffset));
			glEnableVertexAttribArray(5);
			glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride,
				reinterpret_cast<const void *>(flatColorOffset));
		}
		else
		{
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(5);
		}

		if (geometry.hasNormal)
		{
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
				reinterpret_cast<const void *>(normalOffset));
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, stride,
				reinterpret_cast<const void *>(flatNormalOffset));
		}
		else
		{
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(6);
		}

		if (geometry.hasTexCoord)
		{
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride,
				reinterpret_cast<const void *>(texCoordOffset));
		}
		else
			glDisableVertexAttribArray(3);

		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride,
			reinterpret_cast<const void *>(flatPositionOffset));

		const std::size_t drawSlotOffset = expanded ?
			offsetof(CoreGPUFlatVertex, drawSlot) : offsetof(CoreGPUVertex, drawSlot);
		glEnableVertexAttribArray(7);
		glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT, stride,
			reinterpret_cast<const void *>(drawSlotOffset));
	}

	static void applyCurrentAttributes(const ResolvedDraw &command)
	{
		const Geometry &geometry = *command.geometry;
		if (!geometry.hasColor)
		{
			glVertexAttrib4f(1, command.currentAttributes.r, command.currentAttributes.g,
				command.currentAttributes.b, command.currentAttributes.a);
			glVertexAttrib4f(5, command.currentAttributes.r, command.currentAttributes.g,
				command.currentAttributes.b, command.currentAttributes.a);
		}
		if (!geometry.hasNormal)
		{
			glVertexAttrib3f(2, command.currentAttributes.nx, command.currentAttributes.ny,
				command.currentAttributes.nz);
			glVertexAttrib3f(6, command.currentAttributes.nx, command.currentAttributes.ny,
				command.currentAttributes.nz);
		}
		if (!geometry.hasTexCoord)
			glVertexAttrib2f(3, command.currentAttributes.s, command.currentAttributes.t);
	}

	void initialize()
	{
		if (initialized)
			return;
		// The backend's own requirements are OpenGL 3.3: every entry point it
		// calls is core in 3.3 except glBufferStorage, which is optional and
		// selected at runtime. RGBA8 is a required colour-renderable format in
		// 3.3, so it is not queried; glGetInternalformativ is 4.2 and asking
		// for it would have been a version gate with no verified reason.
		if (!GLAD_GL_VERSION_3_3 || glad_glCreateShader == nullptr ||
			glad_glGenVertexArrays == nullptr || glad_glGenBuffers == nullptr ||
			glad_glTexImage2D == nullptr || glad_glProvokingVertex == nullptr ||
			glad_glDrawArrays == nullptr || glad_glReadPixels == nullptr)
		{
			throw std::runtime_error("OpenGL Core backend is missing required functions");
		}

		// The compact vertex layout emits each primitive's legacy provoking
		// vertex last (OpenGL 3.3 core 2.18, table 2.14). LAST_VERTEX_CONVENTION
		// is the initial state, set explicitly so the layout never depends on it.
		glProvokingVertex(GL_LAST_VERTEX_CONVENTION);

		program = coreCreateProgram();
		if (program == 0)
			std::exit(EXIT_FAILURE);

		const GLuint block = glGetUniformBlockIndex(program, "LegacyFFPBlock");
		GLint blockSize = 0;
		if (block != GL_INVALID_INDEX)
			glGetActiveUniformBlockiv(program, block, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (block == GL_INVALID_INDEX || blockSize != static_cast<GLint>(sizeof(CoreGPUState)))
		{
			std::fprintf(stderr, "LegacyGL gl33: shader block ABI mismatch (driver=%d, cpu=%zu)\n",
				blockSize, sizeof(CoreGPUState));
			std::exit(EXIT_FAILURE);
		}
		if (!coreValidateUniformLayout(program))
			std::exit(EXIT_FAILURE);
		glUniformBlockBinding(program, block, 0);
		const GLuint batchBlock = glGetUniformBlockIndex(program, "LegacyBatchBlock");
		if (batchBlock == GL_INVALID_INDEX)
		{
			std::fprintf(stderr, "LegacyGL gl33: LegacyBatchBlock missing from program\n");
			std::exit(EXIT_FAILURE);
		}
		glUniformBlockBinding(program, batchBlock, 1);
		glGenBuffers(1, &batchUniformBuffer);
		// The sampler's texture unit came from a 4.20 `binding` qualifier; in
		// 3.30 it is assigned here instead. Sampler uniforms default to unit 0,
		// so this is explicit rather than load-bearing.
		glUseProgram(program);
		const GLint samplerLocation = glGetUniformLocation(program, "uTextureSampler");
		if (samplerLocation >= 0)
			glUniform1i(samplerLocation, 0);

		glGenVertexArrays(1, &vertexArray);
		glGenBuffers(1, &vertexBuffer);

		// Only the bound offset has to satisfy the implementation alignment; the
		// bound range stays the exact block size so no padding is ever copied.
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformAlignment);
		if (uniformAlignment <= 0)
			uniformAlignment = 256;
		const std::size_t alignment = static_cast<std::size_t>(uniformAlignment);
		uniformStride = (sizeof(CoreGPUState) + alignment - 1) / alignment * alignment;
		uniformSlots = 4096;
		uniformMode = selectUniformMode();
		glGenBuffers(1, &uniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
		if (uniformMode == CoreUniformMode::Persistent)
		{
			// State slots for every region, then a batch-block arena for every
			// region, in one mapping.
			batchArenaBase = static_cast<GLsizeiptr>(uniformStride * uniformSlots * CORE_UNIFORM_REGIONS);
			const GLsizeiptr total = batchArenaBase +
				static_cast<GLsizeiptr>(sizeof(CoreBatchBlock) * CORE_BATCH_BLOCKS * CORE_UNIFORM_REGIONS);
			const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
				GL_MAP_COHERENT_BIT;
			glBufferStorage(GL_UNIFORM_BUFFER, total, nullptr, flags);
			uniformMapped = static_cast<unsigned char *>(
				glMapBufferRange(GL_UNIFORM_BUFFER, 0, total, flags));
			if (uniformMapped == nullptr)
			{
				// An immutable store cannot be reallocated, so start over.
				std::fprintf(stderr,
					"LegacyGL gl33: persistent uniform mapping failed; using legacy uploads\n");
				glDeleteBuffers(1, &uniformBuffer);
				glGenBuffers(1, &uniformBuffer);
				glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
				uniformMode = CoreUniformMode::Legacy;
			}
		}
		if (uniformMode == CoreUniformMode::Legacy)
		{
			glBufferData(GL_UNIFORM_BUFFER, sizeof(CoreGPUState), nullptr, GL_STREAM_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBuffer);
		}

		// The transient vertex ring uses the same storage and mapping as the
		// uniforms, so it exists exactly when persistent uniforms do.
		if (uniformMode == CoreUniformMode::Persistent)
		{
			const GLsizeiptr total = CORE_STREAM_REGION_BYTES *
				static_cast<GLsizeiptr>(CORE_UNIFORM_REGIONS);
			const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
				GL_MAP_COHERENT_BIT;
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferStorage(GL_ARRAY_BUFFER, total, nullptr, flags);
			streamMapped = static_cast<unsigned char *>(
				glMapBufferRange(GL_ARRAY_BUFFER, 0, total, flags));
			if (streamMapped == nullptr)
			{
				std::fprintf(stderr,
					"LegacyGL gl33: persistent vertex mapping failed; using glBufferData\n");
				glDeleteBuffers(1, &vertexBuffer);
				glGenBuffers(1, &vertexBuffer);
			}
		}

		glGenTextures(1, &fallbackTexture);
		glBindTexture(GL_TEXTURE_2D, fallbackTexture);
		glActiveTexture(GL_TEXTURE0);
		textureBindingValid = false;
		const unsigned char black[4] = { 0, 0, 0, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
		std::fprintf(stderr, "LegacyGL gl33: aliased line width range %.3g..%.3g; width 2 is %s\n",
			lineWidthRange[0], lineWidthRange[1],
			(lineWidthRange[0] <= 2.0f && lineWidthRange[1] >= 2.0f) ? "native" : "fallback-to-1");
		std::fprintf(stdout,
			"LegacyGL gl33: capability_report profile=core texture_storage=GL_RGBA8"
			" sampled=native transfer_src=native transfer_dst=native"
			" line_width=%s legacy_dither=driver resource_state=driver-managed"
			" uniforms=%s buffer_storage=%s\n",
			(lineWidthRange[0] <= 2.0f && lineWidthRange[1] >= 2.0f) ?
				"native" : "width1-fallback",
			uniformMode == CoreUniformMode::Persistent ? "persistent" : "legacy",
			GLAD_GL_VERSION_4_4 != 0 ? "core44" :
				(GLAD_GL_ARB_buffer_storage != 0 ? "arb" : "none"));
		initialized = true;
	}

	CoreTexture &ensureTexture(unsigned int name)
	{
		auto found = textures.find(name);
		if (found != textures.end())
			return found->second;

		CoreTexture object;
		glGenTextures(1, &object.handle);
		auto inserted = textures.emplace(name, object);
		return inserted.first->second;
	}

	void ensureTextureStorage(CoreTexture &texture, const ResolvedTextureState &state, bool useGutter)
	{
		unsigned char border[4];
		for (int i = 0; i < 4; i++)
			border[i] = coreFloatByte(state.borderColor[i]);

		if (!texture.derivedDirty && texture.derivedHasGutter == useGutter &&
			(!useGutter || (texture.derivedWrapS == state.wrapS && texture.derivedWrapT == state.wrapT &&
			std::memcmp(texture.derivedBorder, border, 4) == 0)))
			return;

		CoreTextureLevel &source = texture.levels[0];
		if (!source.defined || source.width <= 0 || source.height <= 0)
			return;

		if (!useGutter)
		{
			glBindTexture(GL_TEXTURE_2D, texture.handle);
			textureBindingValid = false;
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source.width, source.height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, source.rgba.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			texture.derivedHasGutter = false;
			texture.derivedDirty = false;
			return;
		}

		const int derivedWidth = source.width + 2;
		const int derivedHeight = source.height + 2;
		std::vector<unsigned char> derived(static_cast<std::size_t>(derivedWidth) *
			static_cast<std::size_t>(derivedHeight) * 4);

		for (int y = 0; y < derivedHeight; y++)
		{
			const bool outsideT = y == 0 || y == derivedHeight - 1;
			int sourceY = y - 1;
			if (y == 0)
				sourceY = state.wrapT == GL_REPEAT ? source.height - 1 : 0;
			else if (y == derivedHeight - 1)
				sourceY = state.wrapT == GL_REPEAT ? 0 : source.height - 1;

			for (int x = 0; x < derivedWidth; x++)
			{
				const bool outsideS = x == 0 || x == derivedWidth - 1;
				int sourceX = x - 1;
				if (x == 0)
					sourceX = state.wrapS == GL_REPEAT ? source.width - 1 : 0;
				else if (x == derivedWidth - 1)
					sourceX = state.wrapS == GL_REPEAT ? 0 : source.width - 1;

				unsigned char *destination = derived.data() +
					(static_cast<std::size_t>(y) * static_cast<std::size_t>(derivedWidth) + static_cast<std::size_t>(x)) * 4;
				if ((outsideS && state.wrapS == GL_CLAMP) || (outsideT && state.wrapT == GL_CLAMP))
					std::memcpy(destination, border, 4);
				else
					std::memcpy(destination, source.rgba.data() +
						(static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(source.width) +
						static_cast<std::size_t>(sourceX)) * 4, 4);
			}
		}

		glBindTexture(GL_TEXTURE_2D, texture.handle);
		textureBindingValid = false;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, derivedWidth, derivedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, derived.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		texture.derivedWrapS = state.wrapS;
		texture.derivedWrapT = state.wrapT;
		std::memcpy(texture.derivedBorder, border, 4);
		texture.derivedHasGutter = true;
		texture.derivedDirty = false;
	}

	// Binds the one texture unit the fixed-function pipeline exposes, skipping
	// calls the driver already has. 26.2 caches the same things in
	// GlStateManager; terrain replays thousands of draws that all share one
	// texture and sampler.
	void bindTextureUnit(GLuint handle, GLuint sampler)
	{
		if (!textureBindingValid || boundTexture != handle)
		{
			glBindTexture(GL_TEXTURE_2D, handle);
			boundTexture = handle;
		}
		if (!textureBindingValid || boundSampler != sampler)
		{
			glBindSampler(0, sampler);
			boundSampler = sampler;
		}
		textureBindingValid = true;
	}

	void bindTexture(const ResolvedDraw &command, CoreGPUState &state)
	{
		if (!command.enables.texture2D || !command.texture.complete)
		{
			bindTextureUnit(fallbackTexture, 0);
			return;
		}

		CoreTexture &texture = ensureTexture(command.texture.name);
		CoreTextureLevel &level = texture.levels[0];
		if (!level.defined)
		{
			bindTextureUnit(fallbackTexture, 0);
			state.flags3[2] = 0;
			return;
		}
		const GLenum minFilter = coreBaseMinFilter(command.texture.minFilter);
		const GLenum magFilter = command.texture.magFilter == GL_NEAREST ? GL_NEAREST : GL_LINEAR;
		const bool useGutter = command.texture.wrapS == GL_CLAMP || command.texture.wrapT == GL_CLAMP;
		ensureTextureStorage(texture, command.texture, useGutter);

		if (texture.sampler == 0)
			glGenSamplers(1, &texture.sampler);
		const GLenum wrapS = useGutter ? GL_CLAMP_TO_EDGE : coreSamplerWrap(command.texture.wrapS);
		const GLenum wrapT = useGutter ? GL_CLAMP_TO_EDGE : coreSamplerWrap(command.texture.wrapT);
		if (texture.samplerMinFilter != minFilter)
		{
			glSamplerParameteri(texture.sampler, GL_TEXTURE_MIN_FILTER, minFilter);
			texture.samplerMinFilter = minFilter;
		}
		if (texture.samplerMagFilter != magFilter)
		{
			glSamplerParameteri(texture.sampler, GL_TEXTURE_MAG_FILTER, magFilter);
			texture.samplerMagFilter = magFilter;
		}
		if (texture.samplerWrapS != wrapS)
		{
			glSamplerParameteri(texture.sampler, GL_TEXTURE_WRAP_S, wrapS);
			texture.samplerWrapS = wrapS;
		}
		if (texture.samplerWrapT != wrapT)
		{
			glSamplerParameteri(texture.sampler, GL_TEXTURE_WRAP_T, wrapT);
			texture.samplerWrapT = wrapT;
		}
		bindTextureUnit(texture.handle, texture.sampler);
		state.textureSize[0] = static_cast<float>(level.width);
		state.textureSize[1] = static_cast<float>(level.height);
		state.textureSize[2] = static_cast<float>(level.width + 2);
		state.textureSize[3] = static_cast<float>(level.height + 2);
		state.flags3[3] = useGutter ? 1u : 0u;
	}

	void fillGPUState(const ResolvedDraw &command, CoreGPUState &state)
	{
		std::memcpy(state.modelView, command.modelView.m, sizeof(state.modelView));
		std::memcpy(state.projection, command.projection.m, sizeof(state.projection));
		std::memcpy(state.texture, command.textureMatrix.m, sizeof(state.texture));
		std::memcpy(state.normal, command.normal.m, sizeof(state.normal));
		coreCopy4(state.globalAmbient, command.lighting.modelAmbient);
		coreCopyMaterial(state.frontMaterial, command.lighting.frontMaterial);
		coreCopyMaterial(state.backMaterial, command.lighting.backMaterial);

		unsigned int lightMask = 0;
		unsigned int lightCount = 0;
		for (int i = 0; i < 8; i++)
		{
			const LightState &source = command.lighting.lights[i];
			CoreGPULight &destination = state.lights[i];
			coreCopy4(destination.ambient, source.ambient);
			coreCopy4(destination.diffuse, source.diffuse);
			coreCopy4(destination.specular, source.specular);
			coreCopy4(destination.positionEye, source.positionEye);
			destination.spotDirectionCutoff[0] = source.spotDirectionEye[0];
			destination.spotDirectionCutoff[1] = source.spotDirectionEye[1];
			destination.spotDirectionCutoff[2] = source.spotDirectionEye[2];
			destination.spotDirectionCutoff[3] = source.spotCutoff == 180.0f ? -1.0f :
				std::cos(source.spotCutoff * 0.01745329251994329577f);
			destination.attenuationExponent[0] = source.constantAttenuation;
			destination.attenuationExponent[1] = source.linearAttenuation;
			destination.attenuationExponent[2] = source.quadraticAttenuation;
			destination.attenuationExponent[3] = source.spotExponent;
			if (source.enabled)
			{
				lightMask |= 1u << i;
				lightCount = static_cast<unsigned int>(i) + 1u;
			}
		}

		coreCopy4(state.fogColor, command.fog.color);
		state.fogParams[0] = command.fog.start;
		state.fogParams[1] = command.fog.end;
		state.fogParams[2] = command.fog.density;
		state.fogParams[3] = command.pipeline.alphaReference;
		state.normalParams[0] = command.normalRescaleFactor;

		state.flags0[0] = command.enables.lighting ? 1u : 0u;
		state.flags0[1] = command.enables.texture2D ? 1u : 0u;
		state.flags0[2] = command.enables.colorMaterial ? 1u : 0u;
		// Bit 0: GL_FLAT. Bit 1: the draw's vertex layout carries separate flat
		// attributes (legacy quads); otherwise the flat inputs alias the smooth
		// ones and the shader computes the primary colour once.
		state.flags0[3] = (command.pipeline.shadeModel == GL_FLAT ? 1u : 0u) |
			(coreLayoutForBatch(*command.primitives) == CoreVertexLayout::Expanded ? 2u : 0u);
		state.flags1[0] = command.enables.normalize ? 2u : (command.enables.rescaleNormal ? 1u : 0u);
		// Low byte: the enabled-light mask. Bits 8+: one past the highest
		// enabled light, so the shader loop stops there. Alpha enables at most
		// GL_LIGHT0 and GL_LIGHT1, which makes that two iterations, not eight.
		state.flags1[1] = lightMask | (lightCount << 8);
		state.flags1[2] = coreFogMode(command.fog.mode, command.enables.fog);
		state.flags1[3] = command.fog.distanceMode == GL_EYE_RADIAL_NV ? 2u :
			(command.fog.distanceMode == GL_EYE_PLANE ? 1u : 0u);
		state.flags2[0] = command.enables.alphaTest ? 1u : 0u;
		state.flags2[1] = coreAlphaFunction(command.pipeline.alphaFunction);
		state.flags2[2] = coreWrapMode(command.texture.wrapS);
		state.flags2[3] = coreWrapMode(command.texture.wrapT);
		state.flags3[0] = coreColorMaterialFace(command.lighting.colorMaterialFace);
		state.flags3[1] = coreColorMaterialMode(command.lighting.colorMaterialMode);
		state.flags3[2] = command.texture.complete ? 1u : 0u;
	}

	void applyPipeline(const ResolvedDraw &command)
	{
		if (pipelineStateValid && coreSameEnables(appliedEnables, command.enables) &&
			coreSamePipeline(appliedPipeline, command.pipeline))
		{
			return;
		}
		appliedEnables = command.enables;
		appliedPipeline = command.pipeline;
		pipelineStateValid = true;

		glViewport(command.pipeline.viewport[0], command.pipeline.viewport[1],
			command.pipeline.viewport[2], command.pipeline.viewport[3]);
		glDepthMask(command.pipeline.depthWrite);
		glColorMask(command.pipeline.colorWrite[0], command.pipeline.colorWrite[1],
			command.pipeline.colorWrite[2], command.pipeline.colorWrite[3]);

		if (command.enables.depthTest)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(command.pipeline.depthFunction);
		}
		else glDisable(GL_DEPTH_TEST);

		if (command.enables.cullFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(command.pipeline.cullFaceMode);
		}
		else glDisable(GL_CULL_FACE);
		glFrontFace(command.pipeline.frontFaceMode);

		if (command.enables.colorLogicOp)
		{
			glDisable(GL_BLEND);
			glEnable(GL_COLOR_LOGIC_OP);
			glLogicOp(command.pipeline.logicOpcode);
		}
		else
		{
			glDisable(GL_COLOR_LOGIC_OP);
			if (command.enables.blend)
			{
				glEnable(GL_BLEND);
				glBlendFunc(command.pipeline.blendSource, command.pipeline.blendDestination);
			}
			else glDisable(GL_BLEND);
		}

		if (command.enables.polygonOffsetFill)
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(command.pipeline.polygonOffsetFactor, command.pipeline.polygonOffsetUnits);
		}
		else glDisable(GL_POLYGON_OFFSET_FILL);

		if (command.enables.scissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
		if (command.enables.stencilTest) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
		if (command.enables.dither) glEnable(GL_DITHER); else glDisable(GL_DITHER);

		const float requestedWidth = command.pipeline.lineWidth;
		if (requestedWidth >= lineWidthRange[0] && requestedWidth <= lineWidthRange[1])
			glLineWidth(requestedWidth);
		else
		{
			glLineWidth(1.0f);
			if (!lineWidthFallbackReported)
			{
				std::fprintf(stderr, "LegacyGL gl33: line width %.3g is unavailable; using classified fallback width 1\n",
					requestedWidth);
				lineWidthFallbackReported = true;
			}
		}
	}

	bool initialized = false;
	GLuint program = 0;
	GLuint vertexArray = 0;
	GLuint vertexBuffer = 0;
	GLuint uniformBuffer = 0;
	// Pending glMultiDrawArrays over resident entries of one page: see
	// tryBatchResidentDraw. Matrices are indexed by entry slot.
	GLuint batchUniformBuffer = 0;
	GLsizeiptr batchArenaBase = 0;
	std::size_t batchBlockCursor = 0;
	std::uint64_t batchBlockOverflows = 0;
	std::uint64_t batchPageRuns = 0;
	std::size_t batchCount = 0;
	Topology batchTopology = Topology::Triangles;
	CoreGPUState batchState = {};
	ResolvedDraw batchCommand;
	CoreBatchedDraw batchDraws[CORE_BATCH_SLOTS];
	GLint batchFirst[CORE_BATCH_SLOTS] = {};
	GLsizei batchCounts[CORE_BATCH_SLOTS] = {};
	CoreBatchBlock batchBlock;
	std::uint64_t batchedDrawCalls = 0;
	std::uint64_t batchedDraws = 0;
	std::uint64_t batchBreakTopology = 0;
	std::uint64_t batchBreakState = 0;
	std::uint64_t batchBreakOther = 0;
	GLuint fallbackTexture = 0;
	GLuint boundTexture = 0;
	GLuint boundSampler = 0;
	bool textureBindingValid = false;
	float lineWidthRange[2] = { 1.0f, 1.0f };
	bool lineWidthFallbackReported = false;
	ResolvedEnableState appliedEnables;
	ResolvedPipelineState appliedPipeline;
	// Uniform arena. Every draw writes its own disjoint, alignment-correct slot
	// instead of overwriting one live range, which is what forced the driver to
	// resolve a write-after-read hazard roughly 3,000 times per frame. The
	// arena is orphaned once per frame, never per draw.
	GLint uniformAlignment = 256;
	std::size_t uniformStride = 0;
	std::size_t uniformSlots = 0;
	std::size_t uniformCursor = 0;
	std::size_t uniformPeakSlots = 0;
	CoreUniformMode uniformMode = CoreUniformMode::Persistent;
	unsigned char *uniformMapped = nullptr;
	GLsync uniformFences[CORE_UNIFORM_REGIONS] = {};
	std::size_t uniformRegion = 0;
	std::uint64_t uniformOverflows = 0;
	std::uint64_t uniformFenceWaits = 0;
	// Transient vertex ring, mapped alongside the uniforms and sharing their
	// region and fence; null when persistent storage is unavailable, in which
	// case the transient path falls back to glBufferData.
	unsigned char *streamMapped = nullptr;
	GLsizeiptr streamCursor = 0;
	GLsizeiptr streamPeakBytes = 0;
	std::uint64_t streamOverflows = 0;
	// Resident misses uploaded through the stream (staged) versus straight
	// into their page (direct); the difference is what the Switch stalls on.
	std::uint64_t residentStagedUploads = 0;
	std::uint64_t residentDirectUploads = 0;
	// Mutable buffer for the rare transient draw larger than a ring region.
	GLuint streamOverflowBuffer = 0;
	bool pipelineStateValid = false;
	CoreLogicalNameAllocator textureNames;
	CoreLogicalNameAllocator bufferNames;
	std::uint64_t nextListName = 1;
	std::map<unsigned int, CoreTexture> textures;
	std::map<std::uint64_t, CoreResidentGeometryEntry> residentGeometry;
	std::vector<CoreResidentPage> residentPages;
	// Skips the rebind when consecutive draws share a page's vertex array.
	GLuint boundVertexArray = 0;
	std::uint64_t residentCacheHits = 0;
	std::uint64_t residentCacheMisses = 0;
	std::uint64_t residentGeometryBytes = 0;
	std::uint64_t residentGeometryReleases = 0;
	unsigned int streamAttributeMask = 0;
	CoreVertexLayout streamVertexLayout = CoreVertexLayout::Compact;
	bool streamLayoutValid = false;
};

static CoreGLSink theCoreGLSink;

}

namespace renderbackend
{

static const Configuration &openGL33Configuration()
{
	static const Configuration config = {
		"gl33",
		3, 3, OpenGLProfile::Core,
		3, 3, OpenGLProfile::Core,
		true,
		false
	};
	return config;
}

static void openGL33Initialize()
{
	openglbackend::initialize(openGL33Configuration());
}

static void openGL33Present()
{
	legacygl::theCoreGLSink.flushBatch();
	openglbackend::present();
	// Frame boundary: recycle the uniform arena here, never in the draw path.
	legacygl::theCoreGLSink.beginFrame();
}

static void openGL33Shutdown()
{
	legacygl::theCoreGLSink.shutdown();
	openglbackend::shutdown();
}

static bool openGL33HasCapability(const char *capability)
{
	return openglbackend::hasCapability(capability);
}

static legacygl::Sink *openGL33Sink()
{
	return &legacygl::theCoreGLSink;
}

const Backend &openGL33Backend()
{
	static const Backend backend = {
		"gl33",
		openGL33Configuration,
		openGL33Initialize,
		openGL33Present,
		openGL33Shutdown,
		openGL33HasCapability,
		openGL33Sink
	};
	return backend;
}

}
