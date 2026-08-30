// OpenGL 4.6 Core backend for the LegacyGL semantic frontend.
//
// This file consumes only resolved commands. The raw Sink callbacks remain
// no-ops (apart from object-name allocation and glFinish), so fixed-function
// state is owned and interpreted exactly once by Context.

#include <glad/glad.h>

#include "backends/Backend.h"
#include "backends/OpenGL/Context.h"
#include "backends/OpenGL46/Shaders.h"
#include "legacygl/Sink.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace legacygl
{

static const int CORE_TEXTURE_LEVELS = 16;

struct CoreGPUVertex
{
	float position[3];
	float color[4];
	float normal[3];
	float texCoord[2];
	float flatPosition[3];
	float flatColor[4];
	float flatNormal[3];
};

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
};

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
			std::fprintf(stderr, "LegacyGL gl46: shader block ABI member '%s' is inactive or missing\n",
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
				"LegacyGL gl46: shader block ABI member '%s' offset mismatch (driver=%d, cpu=%d)\n",
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

static int corePixelComponents(unsigned int format)
{
	switch (format)
	{
		case GL_RGBA:
		case GL_BGRA:
			return 4;
		case GL_RGB:
		case GL_BGR:
			return 3;
		case GL_LUMINANCE_ALPHA:
			return 2;
		case GL_ALPHA:
		case GL_LUMINANCE:
			return 1;
		default:
			return 0;
	}
}

static std::size_t coreAlignedRowSize(std::size_t rowSize, int alignment)
{
	const std::size_t value = static_cast<std::size_t>(alignment);
	return (rowSize + value - 1) / value * value;
}

static void coreDecodePixel(const unsigned char *source, unsigned int format, unsigned char *rgba)
{
	switch (format)
	{
		case GL_RGBA:
			rgba[0] = source[0]; rgba[1] = source[1]; rgba[2] = source[2]; rgba[3] = source[3];
			break;
		case GL_BGRA:
			rgba[0] = source[2]; rgba[1] = source[1]; rgba[2] = source[0]; rgba[3] = source[3];
			break;
		case GL_RGB:
			rgba[0] = source[0]; rgba[1] = source[1]; rgba[2] = source[2]; rgba[3] = 255;
			break;
		case GL_BGR:
			rgba[0] = source[2]; rgba[1] = source[1]; rgba[2] = source[0]; rgba[3] = 255;
			break;
		case GL_LUMINANCE_ALPHA:
			rgba[0] = source[0]; rgba[1] = source[0]; rgba[2] = source[0]; rgba[3] = source[1];
			break;
		case GL_ALPHA:
			rgba[0] = 255; rgba[1] = 255; rgba[2] = 255; rgba[3] = source[0];
			break;
		default:
			rgba[0] = source[0]; rgba[1] = source[0]; rgba[2] = source[0]; rgba[3] = 255;
			break;
	}
}

static void coreEncodePixel(const unsigned char *rgba, unsigned int format, unsigned char *destination)
{
	switch (format)
	{
		case GL_RGBA:
			destination[0] = rgba[0]; destination[1] = rgba[1]; destination[2] = rgba[2]; destination[3] = rgba[3];
			break;
		case GL_BGRA:
			destination[0] = rgba[2]; destination[1] = rgba[1]; destination[2] = rgba[0]; destination[3] = rgba[3];
			break;
		case GL_RGB:
			destination[0] = rgba[0]; destination[1] = rgba[1]; destination[2] = rgba[2];
			break;
		case GL_BGR:
			destination[0] = rgba[2]; destination[1] = rgba[1]; destination[2] = rgba[0];
			break;
		case GL_LUMINANCE_ALPHA:
			destination[0] = rgba[0]; destination[1] = rgba[3];
			break;
		case GL_ALPHA:
			destination[0] = rgba[3];
			break;
		default:
			destination[0] = rgba[0];
			break;
	}
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
	std::fprintf(stderr, "LegacyGL gl46 shader compile failed: %s\n", log.data());
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
	std::fprintf(stderr, "LegacyGL gl46 shader link failed: %s\n", log.data());
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

static CoreGPUVertex coreGPUVertex(const Vertex &vertex, const Vertex &flat)
{
	CoreGPUVertex result;
	result.position[0] = vertex.x; result.position[1] = vertex.y; result.position[2] = vertex.z;
	result.color[0] = vertex.r; result.color[1] = vertex.g; result.color[2] = vertex.b; result.color[3] = vertex.a;
	result.normal[0] = vertex.nx; result.normal[1] = vertex.ny; result.normal[2] = vertex.nz;
	result.texCoord[0] = vertex.s; result.texCoord[1] = vertex.t;
	result.flatPosition[0] = flat.x; result.flatPosition[1] = flat.y; result.flatPosition[2] = flat.z;
	result.flatColor[0] = flat.r; result.flatColor[1] = flat.g; result.flatColor[2] = flat.b; result.flatColor[3] = flat.a;
	result.flatNormal[0] = flat.nx; result.flatNormal[1] = flat.ny; result.flatNormal[2] = flat.nz;
	return result;
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
				std::fprintf(stderr, "LegacyGL gl46: logical texture-name namespace exhausted\n");
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
				std::fprintf(stderr, "LegacyGL gl46: logical buffer-name namespace exhausted\n");
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

	void resolvedDraw(const ResolvedDraw &command) override
	{
		initialize();
		if (command.geometry == nullptr || command.primitives == nullptr || command.geometry->vertices.empty())
			return;

		std::vector<CoreGPUVertex> vertices;
		const int verticesPerPrimitive = command.primitives->topology == Topology::Points ? 1 :
			(command.primitives->topology == Topology::Lines ? 2 : 3);
		vertices.reserve(command.primitives->primitives.size() * static_cast<std::size_t>(verticesPerPrimitive));
		for (const CanonicalPrimitive &primitive : command.primitives->primitives)
		{
			const Vertex &flat = command.geometry->vertices[static_cast<std::size_t>(primitive.provoking)];
			for (int i = 0; i < verticesPerPrimitive; i++)
			{
				const Vertex &vertex = command.geometry->vertices[static_cast<std::size_t>(primitive.indices[i])];
				vertices.push_back(coreGPUVertex(vertex, flat));
			}
		}
		if (vertices.empty())
			return;

		CoreGPUState state = {};
		fillGPUState(command, state);
		applyPipeline(command);
		bindTexture(command, state);

		glUseProgram(program);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(state), &state);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBuffer);

		glBindVertexArray(vertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(CoreGPUVertex)),
			vertices.data(), GL_STREAM_DRAW);
		glDrawArrays(coreTopology(command.primitives->topology), 0, static_cast<GLsizei>(vertices.size()));
	}

	void resolvedClear(const ResolvedClear &command) override
	{
		initialize();
		glColorMask(command.colorWrite[0], command.colorWrite[1], command.colorWrite[2], command.colorWrite[3]);
		glDepthMask(command.depthWrite);
		if (command.scissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
		if (command.dither) glEnable(GL_DITHER); else glDisable(GL_DITHER);
		glClearColor(command.color[0], command.color[1], command.color[2], command.color[3]);
		glClearDepth(command.depth);
		glClear(command.mask);
	}

	void resolvedTextureUpload(const ResolvedTextureUpload &command) override
	{
		initialize();
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

		if (command.pixels != nullptr && command.width > 0 && command.height > 0)
		{
			const int components = corePixelComponents(command.sourceFormat);
			const std::size_t sourceRow = coreAlignedRowSize(
				static_cast<std::size_t>(command.width) * static_cast<std::size_t>(components), command.unpackAlignment);
			const unsigned char *source = static_cast<const unsigned char *>(command.pixels);
			for (int y = 0; y < command.height; y++)
			{
				for (int x = 0; x < command.width; x++)
				{
					unsigned char rgba[4];
					coreDecodePixel(source + static_cast<std::size_t>(y) * sourceRow +
						static_cast<std::size_t>(x) * static_cast<std::size_t>(components), command.sourceFormat, rgba);
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
		if (command.width == 0 || command.height == 0)
			return;

		std::vector<unsigned char> rgba(static_cast<std::size_t>(command.width) *
			static_cast<std::size_t>(command.height) * 4);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(command.x, command.y, command.width, command.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

		const int components = corePixelComponents(command.format);
		const std::size_t destinationRow = coreAlignedRowSize(
			static_cast<std::size_t>(command.width) * static_cast<std::size_t>(components), command.packAlignment);
		unsigned char *destination = static_cast<unsigned char *>(command.pixels);
		for (int y = 0; y < command.height; y++)
		{
			for (int x = 0; x < command.width; x++)
			{
				const unsigned char *source = rgba.data() +
					(static_cast<std::size_t>(y) * static_cast<std::size_t>(command.width) + static_cast<std::size_t>(x)) * 4;
				coreEncodePixel(source, command.format, destination + static_cast<std::size_t>(y) * destinationRow +
					static_cast<std::size_t>(x) * static_cast<std::size_t>(components));
			}
		}
	}

private:
	void initialize()
	{
		if (initialized)
			return;

		program = coreCreateProgram();
		if (program == 0)
			std::exit(EXIT_FAILURE);

		const GLuint block = glGetUniformBlockIndex(program, "LegacyFFPBlock");
		GLint blockSize = 0;
		if (block != GL_INVALID_INDEX)
			glGetActiveUniformBlockiv(program, block, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (block == GL_INVALID_INDEX || blockSize != static_cast<GLint>(sizeof(CoreGPUState)))
		{
			std::fprintf(stderr, "LegacyGL gl46: shader block ABI mismatch (driver=%d, cpu=%zu)\n",
				blockSize, sizeof(CoreGPUState));
			std::exit(EXIT_FAILURE);
		}
		if (!coreValidateUniformLayout(program))
			std::exit(EXIT_FAILURE);
		glUniformBlockBinding(program, block, 0);

		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);
		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, position)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, color)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, normal)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, texCoord)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, flatPosition)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, flatColor)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(CoreGPUVertex),
			reinterpret_cast<const void *>(offsetof(CoreGPUVertex, flatNormal)));

		glGenBuffers(1, &uniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CoreGPUState), nullptr, GL_STREAM_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBuffer);

		glGenTextures(1, &fallbackTexture);
		glBindTexture(GL_TEXTURE_2D, fallbackTexture);
		const unsigned char black[4] = { 0, 0, 0, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
		std::fprintf(stderr, "LegacyGL gl46: aliased line width range %.3g..%.3g; width 2 is %s\n",
			lineWidthRange[0], lineWidthRange[1],
			(lineWidthRange[0] <= 2.0f && lineWidthRange[1] >= 2.0f) ? "native" : "fallback-to-1");
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

	void bindTexture(const ResolvedDraw &command, CoreGPUState &state)
	{
		glActiveTexture(GL_TEXTURE0);
		if (!command.enables.texture2D || !command.texture.complete)
		{
			glBindTexture(GL_TEXTURE_2D, fallbackTexture);
			glBindSampler(0, 0);
			return;
		}

		CoreTexture &texture = ensureTexture(command.texture.name);
		CoreTextureLevel &level = texture.levels[0];
		if (!level.defined)
		{
			glBindTexture(GL_TEXTURE_2D, fallbackTexture);
			glBindSampler(0, 0);
			state.flags3[2] = 0;
			return;
		}
		const GLenum minFilter = coreBaseMinFilter(command.texture.minFilter);
		const GLenum magFilter = command.texture.magFilter == GL_NEAREST ? GL_NEAREST : GL_LINEAR;
		const bool useGutter = command.texture.wrapS == GL_CLAMP || command.texture.wrapT == GL_CLAMP;
		ensureTextureStorage(texture, command.texture, useGutter);

		if (texture.sampler == 0)
			glGenSamplers(1, &texture.sampler);
		glSamplerParameteri(texture.sampler, GL_TEXTURE_MIN_FILTER, minFilter);
		glSamplerParameteri(texture.sampler, GL_TEXTURE_MAG_FILTER, magFilter);
		glSamplerParameteri(texture.sampler, GL_TEXTURE_WRAP_S,
			useGutter ? GL_CLAMP_TO_EDGE : coreSamplerWrap(command.texture.wrapS));
		glSamplerParameteri(texture.sampler, GL_TEXTURE_WRAP_T,
			useGutter ? GL_CLAMP_TO_EDGE : coreSamplerWrap(command.texture.wrapT));
		glBindTexture(GL_TEXTURE_2D, texture.handle);
		glBindSampler(0, texture.sampler);
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
				lightMask |= 1u << i;
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
		state.flags0[3] = command.pipeline.shadeModel == GL_FLAT ? 1u : 0u;
		state.flags1[0] = command.enables.normalize ? 2u : (command.enables.rescaleNormal ? 1u : 0u);
		state.flags1[1] = lightMask;
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
				std::fprintf(stderr, "LegacyGL gl46: line width %.3g is unavailable; using classified fallback width 1\n",
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
	GLuint fallbackTexture = 0;
	float lineWidthRange[2] = { 1.0f, 1.0f };
	bool lineWidthFallbackReported = false;
	CoreLogicalNameAllocator textureNames;
	CoreLogicalNameAllocator bufferNames;
	std::uint64_t nextListName = 1;
	std::map<unsigned int, CoreTexture> textures;
};

static CoreGLSink theCoreGLSink;

}

namespace renderbackend
{

static const Configuration &openGL46Configuration()
{
	static const Configuration config = {
		"gl46",
		4, 6, OpenGLProfile::Core,
		4, 6, OpenGLProfile::Core,
		true,
		false
	};
	return config;
}

static void openGL46Initialize()
{
	openglbackend::initialize(openGL46Configuration());
}

static void openGL46Present()
{
	openglbackend::present();
}

static void openGL46Shutdown()
{
	openglbackend::shutdown();
}

static bool openGL46HasCapability(const char *capability)
{
	return openglbackend::hasCapability(capability);
}

static legacygl::Sink *openGL46Sink()
{
	return &legacygl::theCoreGLSink;
}

const Backend &openGL46Backend()
{
	static const Backend backend = {
		"gl46",
		openGL46Configuration,
		openGL46Initialize,
		openGL46Present,
		openGL46Shutdown,
		openGL46HasCapability,
		openGL46Sink
	};
	return backend;
}

}
