#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL_video.h>

#include "backends/Backend.h"
#include "backends/Platform/Platform.h"
#include "legacygl/LegacyGL.h"
#include "lwjgl/GLContext.h"

namespace gpuparity
{

const int WindowWidth = 128;
const int WindowHeight = 128;
const int LineMaskWidth = 32;
const int LineMaskHeight = 32;
const char *RecordHeader = "a126cpp-legacygl-gpu-parity 4";

struct CaseResult
{
	std::string name;
	std::string value;
};

struct ResultSet
{
	std::string backend;
	std::vector<CaseResult> cases;
};

struct TextureCoordinate
{
	float s;
	float t;
};

struct InterleavedVertex
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat u;
	GLfloat v;
	GLubyte r;
	GLubyte g;
	GLubyte b;
	GLubyte a;
	GLbyte nx;
	GLbyte ny;
	GLbyte nz;
	GLbyte normalPadding;
	GLint unused;
};

static_assert(sizeof(InterleavedVertex) == 32, "Tesselator vertices have a 32-byte stride");
static_assert(offsetof(InterleavedVertex, r) == 20, "Tesselator colours begin at byte 20");

typedef std::array<unsigned char, 3> Rgb;
typedef std::array<unsigned char, 4> Rgba;

std::string byteHex(const std::vector<unsigned char> &bytes)
{
	std::ostringstream result;
	result << std::hex << std::setfill('0');
	for (unsigned char byte : bytes)
		result << std::setw(2) << static_cast<unsigned int>(byte);
	return result.str();
}

std::string byteHex(const Rgb &bytes)
{
	return byteHex(std::vector<unsigned char>(bytes.begin(), bytes.end()));
}

std::string byteHex(const Rgba &bytes)
{
	return byteHex(std::vector<unsigned char>(bytes.begin(), bytes.end()));
}

std::vector<unsigned char> parseHex(const std::string &value, const std::string &caseName)
{
	if ((value.size() & 1U) != 0)
		throw std::runtime_error("case " + caseName + ": odd-length hexadecimal value");

	std::vector<unsigned char> result;
	result.reserve(value.size() / 2);
	for (std::size_t i = 0; i < value.size(); i += 2)
	{
		const auto digit = [&](char character)
		{
			if (character >= '0' && character <= '9')
				return character - '0';
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
			if (character >= 'a' && character <= 'f')
				return character - 'a' + 10;
			throw std::runtime_error("case " + caseName + ": invalid hexadecimal value");
		};
		result.push_back(static_cast<unsigned char>((digit(value[i]) << 4) | digit(value[i + 1])));
	}
	return result;
}

void addCase(ResultSet &results, const std::string &name, const std::string &value)
{
	for (const CaseResult &existing : results.cases)
	{
		if (existing.name == name)
			throw std::runtime_error("duplicate case " + name);
	}
	results.cases.push_back({ name, value });
}

void addByteCase(ResultSet &results, const std::string &name, const Rgb &value)
{
	addCase(results, name, byteHex(value));
}

void addRgbaCase(ResultSet &results, const std::string &name, const Rgba &value)
{
	addCase(results, name, byteHex(value));
}

void addBooleanCase(ResultSet &results, const std::string &name, bool value, bool enforceExpected)
{
	if (enforceExpected && !value)
		throw std::runtime_error("case " + name + ": expected true");
	addCase(results, name, value ? "1" : "0");
}

void requireExact(const std::string &name, const Rgb &actual, const Rgb &expected)
{
	if (actual != expected)
		throw std::runtime_error("case " + name + ": expected " + byteHex(expected) + ", got " + byteHex(actual));
}

void requireExact(const std::string &name, const Rgba &actual, const Rgba &expected)
{
	if (actual != expected)
		throw std::runtime_error("case " + name + ": expected " + byteHex(expected) + ", got " + byteHex(actual));
}

void requireExact(const std::string &name, const std::vector<unsigned char> &actual,
	const std::vector<unsigned char> &expected)
{
	if (actual != expected)
		throw std::runtime_error("case " + name + ": expected " + byteHex(expected) + ", got " + byteHex(actual));
}

void requireNear(const std::string &name, const Rgb &actual, const Rgb &expected, int tolerance)
{
	for (std::size_t i = 0; i < actual.size(); i++)
	{
		if (std::abs(static_cast<int>(actual[i]) - static_cast<int>(expected[i])) > tolerance)
			throw std::runtime_error("case " + name + ": expected " + byteHex(expected) +
				" within " + std::to_string(tolerance) + ", got " + byteHex(actual));
	}
}

void configure2D(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, static_cast<double>(width), 0.0, static_cast<double>(height), -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_COLOR_LOGIC_OP);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_NORMALIZE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_RESCALE_NORMAL);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_TEXTURE_2D);
	for (int light = 0; light < 8; light++)
		glDisable(GL_LIGHT0 + light);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glShadeModel(GL_SMOOTH);
	glLineWidth(1.0f);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void recordGeneratedNameCases(ResultSet &results, bool enforceExpected)
{
	const GLuint occupied = 1;
	GLuint generated = 0;

	glBindTexture(GL_TEXTURE_2D, occupied);
	glGenTextures(1, &generated);
	const bool textureAvoided = generated != occupied;
	glBindTexture(GL_TEXTURE_2D, 0);
	const GLuint textures[] = { occupied, generated };
	glDeleteTextures(2, textures);
	addBooleanCase(results, "namegen.texture.avoids-bound", textureAvoided, enforceExpected);

	generated = 0;
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, occupied);
	glGenBuffersARB(1, &generated);
	const bool bufferAvoided = generated != occupied;
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	addBooleanCase(results, "namegen.buffer.avoids-bound", bufferAvoided, enforceExpected);

	glNewList(occupied, GL_COMPILE);
	glEndList();
	generated = glGenLists(1);
	const bool listAvoided = generated != occupied;
	glDeleteLists(occupied, 1);
	if (generated != occupied)
		glDeleteLists(generated, 1);
	addBooleanCase(results, "namegen.list.avoids-created", listAvoided, enforceExpected);
}

void drawQuad(float left, float bottom, float right, float top, float z)
{
	glBegin(GL_QUADS);
	glVertex3f(left, bottom, z);
	glVertex3f(right, bottom, z);
	glVertex3f(right, top, z);
	glVertex3f(left, top, z);
	glEnd();
}

void drawTexturedQuad(float left, float bottom, float right, float top, float s, float t)
{
	glBegin(GL_QUADS);
	glTexCoord2f(s, t);
	glVertex3f(left, bottom, 0.0f);
	glTexCoord2f(s, t);
	glVertex3f(right, bottom, 0.0f);
	glTexCoord2f(s, t);
	glVertex3f(right, top, 0.0f);
	glTexCoord2f(s, t);
	glVertex3f(left, top, 0.0f);
	glEnd();
}

Rgb readRgb(int x, int y)
{
	Rgb result = { 0, 0, 0 };
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, result.data());
	return result;
}

Rgba readRgba(int x, int y)
{
	Rgba result = { 0, 0, 0, 0 };
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
	return result;
}

void setColor(const Rgba &color)
{
	glColor4f(color[0] / 255.0f, color[1] / 255.0f,
		color[2] / 255.0f, color[3] / 255.0f);
}

std::vector<Rgb> renderTextureSamples(const std::vector<TextureCoordinate> &coordinates)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	std::vector<Rgb> result;
	for (std::size_t i = 0; i < coordinates.size(); i++)
	{
		const float left = 4.0f + static_cast<float>(i) * 12.0f;
		drawTexturedQuad(left, 4.0f, left + 8.0f, 12.0f, coordinates[i].s, coordinates[i].t);
	}
	glFinish();
	for (std::size_t i = 0; i < coordinates.size(); i++)
		result.push_back(readRgb(8 + static_cast<int>(i) * 12, 8));
	return result;
}

void recordAlphaCases(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_ALPHA_TEST);

	const GLenum functions[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
	const char *functionNames[] = { "never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always" };
	const unsigned int alphaBytes[] = { 127, 128, 129 };
	const bool expected[][3] = {
		{ false, false, false },
		{ true, false, false },
		{ false, true, false },
		{ true, true, false },
		{ false, false, true },
		{ true, false, true },
		{ false, true, true },
		{ true, true, true }
	};

	for (std::size_t function = 0; function < 8; function++)
	{
		glAlphaFunc(functions[function], 128.0f / 255.0f);
		for (std::size_t alpha = 0; alpha < 3; alpha++)
		{
			const int index = static_cast<int>(function * 3 + alpha);
			const float left = 2.0f + static_cast<float>(index % 12) * 10.0f;
			const float bottom = 2.0f + static_cast<float>(index / 12) * 10.0f;
			glColor4f(1.0f, 1.0f, 1.0f, static_cast<float>(alphaBytes[alpha]) / 255.0f);
			drawQuad(left, bottom, left + 6.0f, bottom + 6.0f, 0.0f);
		}
	}
	glFinish();

	for (std::size_t function = 0; function < 8; function++)
	{
		for (std::size_t alpha = 0; alpha < 3; alpha++)
		{
			const int index = static_cast<int>(function * 3 + alpha);
			const Rgb pixel = readRgb(5 + (index % 12) * 10, 5 + (index / 12) * 10);
			const bool passed = pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0;
			const std::string name = "alpha." + std::string(functionNames[function]) + "." + std::to_string(alphaBytes[alpha]);
			if (enforceExpected && passed != expected[function][alpha])
				throw std::runtime_error("case " + name + ": expected " + (expected[function][alpha] ? "pass" : "fail"));
			addCase(results, name, passed ? "1" : "0");
		}
	}
	glDisable(GL_ALPHA_TEST);
}

void drawTriangle(GLenum winding)
{
	glBegin(GL_TRIANGLES);
	glVertex3f(4.0f, 4.0f, 0.0f);
	if (winding == GL_CCW)
	{
		glVertex3f(28.0f, 4.0f, 0.0f);
		glVertex3f(16.0f, 28.0f, 0.0f);
	}
	else
	{
		glVertex3f(16.0f, 28.0f, 0.0f);
		glVertex3f(28.0f, 4.0f, 0.0f);
	}
	glEnd();
}

bool renderCullCase(GLenum winding, GLenum cullFace)
{
	configure2D(32, 32);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(cullFace);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	drawTriangle(winding);
	glFinish();
	const Rgb pixel = readRgb(16, 12);
	return pixel[0] != 0;
}

void recordCullCases(ResultSet &results, bool enforceExpected)
{
	struct CullCase
	{
		const char *name;
		GLenum winding;
		GLenum cullFace;
		bool visible;
	};
	const CullCase cases[] = {
		{ "cull.ccw.back", GL_CCW, GL_BACK, true },
		{ "cull.ccw.front", GL_CCW, GL_FRONT, false },
		{ "cull.cw.back", GL_CW, GL_BACK, false },
		{ "cull.cw.front", GL_CW, GL_FRONT, true }
	};
	for (const CullCase &test : cases)
	{
		const bool visible = renderCullCase(test.winding, test.cullFace);
		if (enforceExpected && visible != test.visible)
			throw std::runtime_error("case " + std::string(test.name) + ": unexpected visibility");
		addCase(results, test.name, visible ? "1" : "0");
	}
}

bool depthComparisonPasses(GLenum function, int relation)
{
	switch (function)
	{
		case GL_NEVER: return false;
		case GL_LESS: return relation < 0;
		case GL_EQUAL: return relation == 0;
		case GL_LEQUAL: return relation <= 0;
		case GL_GREATER: return relation > 0;
		case GL_NOTEQUAL: return relation != 0;
		case GL_GEQUAL: return relation >= 0;
		default: return true;
	}
}

std::vector<unsigned char> renderDepthSignature(GLenum function)
{
	configure2D(48, 16);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_ALWAYS);
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	for (int i = 0; i < 3; i++)
		drawQuad(2.0f + i * 16.0f, 2.0f, 14.0f + i * 16.0f, 14.0f, 0.0f);

	const float candidateDepths[] = { 0.5f, 0.0f, -0.5f };
	glDepthFunc(function);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	for (int i = 0; i < 3; i++)
		drawQuad(2.0f + i * 16.0f, 2.0f, 14.0f + i * 16.0f, 14.0f, candidateDepths[i]);
	glFinish();

	std::vector<unsigned char> result;
	for (int i = 0; i < 3; i++)
	{
		const Rgb pixel = readRgb(8 + i * 16, 8);
		result.insert(result.end(), pixel.begin(), pixel.end());
	}
	return result;
}

Rgb renderDepthMaskCase(bool writeCandidate)
{
	configure2D(16, 16);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_TRUE);
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	drawQuad(2.0f, 2.0f, 14.0f, 14.0f, 0.0f);

	glDepthMask(writeCandidate ? GL_TRUE : GL_FALSE);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	drawQuad(2.0f, 2.0f, 14.0f, 14.0f, 0.5f);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	drawQuad(2.0f, 2.0f, 14.0f, 14.0f, 0.25f);
	glFinish();
	return readRgb(8, 8);
}

void recordDepthCases(ResultSet &results, bool enforceExpected)
{
	const GLenum functions[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
	const char *functionNames[] = { "never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always" };
	const int relations[] = { -1, 0, 1 };
	for (std::size_t functionIndex = 0; functionIndex < 8; functionIndex++)
	{
		const std::string name = "depth." + std::string(functionNames[functionIndex]) + ".signature";
		const std::vector<unsigned char> actual = renderDepthSignature(functions[functionIndex]);
		std::vector<unsigned char> expected;
		for (int relation : relations)
		{
			const Rgb pixel = depthComparisonPasses(functions[functionIndex], relation) ?
				Rgb{ 255, 255, 255 } : Rgb{ 0, 0, 255 };
			expected.insert(expected.end(), pixel.begin(), pixel.end());
		}
		if (enforceExpected)
			requireExact(name, actual, expected);
		addCase(results, name, byteHex(actual));
	}

	const Rgb preserved = renderDepthMaskCase(false);
	const Rgb updated = renderDepthMaskCase(true);
	if (enforceExpected)
	{
		requireExact("depth.mask-false-preserves", preserved, Rgb{ 0, 255, 0 });
		requireExact("depth.mask-true-updates", updated, Rgb{ 255, 0, 0 });
	}
	addByteCase(results, "depth.mask-false-preserves", preserved);
	addByteCase(results, "depth.mask-true-updates", updated);
}

Rgba renderBlendCase(GLenum sourceFactor, GLenum destinationFactor)
{
	const Rgba destination = { 48, 96, 160, 64 };
	const Rgba source = { 192, 128, 32, 160 };
	configure2D(16, 16);
	glClearColor(destination[0] / 255.0f, destination[1] / 255.0f,
		destination[2] / 255.0f, destination[3] / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(sourceFactor, destinationFactor);
	setColor(source);
	drawQuad(2.0f, 2.0f, 14.0f, 14.0f, 0.0f);
	glFinish();
	return readRgba(8, 8);
}

void recordBlendCases(ResultSet &results)
{
	struct BlendCase
	{
		const char *name;
		GLenum source;
		GLenum destination;
	};
	const BlendCase cases[] = {
		{ "blend.game.src-alpha.one-minus-src-alpha", GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA },
		{ "blend.game.src-color.one", GL_SRC_COLOR, GL_ONE },
		{ "blend.game.one.one", GL_ONE, GL_ONE },
		{ "blend.game.src-alpha.one", GL_SRC_ALPHA, GL_ONE },
		{ "blend.game.dst-color.src-color", GL_DST_COLOR, GL_SRC_COLOR },
		{ "blend.game.one-minus-dst-color.one-minus-src-color", GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR },
		{ "blend.factor.src-color.zero", GL_SRC_COLOR, GL_ZERO },
		{ "blend.factor.dst-color.zero", GL_DST_COLOR, GL_ZERO },
		{ "blend.factor.src-alpha-saturate.zero", GL_SRC_ALPHA_SATURATE, GL_ZERO }
	};
	for (const BlendCase &test : cases)
		addRgbaCase(results, test.name, renderBlendCase(test.source, test.destination));
}

unsigned char logicResult(GLenum operation, unsigned char source, unsigned char destination)
{
	switch (operation)
	{
		case GL_CLEAR: return 0;
		case GL_AND: return source & destination;
		case GL_AND_REVERSE: return source & static_cast<unsigned char>(~destination);
		case GL_COPY: return source;
		case GL_AND_INVERTED: return static_cast<unsigned char>(~source) & destination;
		case GL_NOOP: return destination;
		case GL_XOR: return source ^ destination;
		case GL_OR: return source | destination;
		case GL_NOR: return static_cast<unsigned char>(~(source | destination));
		case GL_EQUIV: return static_cast<unsigned char>(~(source ^ destination));
		case GL_INVERT: return static_cast<unsigned char>(~destination);
		case GL_OR_REVERSE: return source | static_cast<unsigned char>(~destination);
		case GL_COPY_INVERTED: return static_cast<unsigned char>(~source);
		case GL_OR_INVERTED: return static_cast<unsigned char>(~source) | destination;
		case GL_NAND: return static_cast<unsigned char>(~(source & destination));
		default: return 255;
	}
}

void recordLogicOpCases(ResultSet &results, bool enforceExpected)
{
	const GLenum operations[] = {
		GL_CLEAR, GL_AND, GL_AND_REVERSE, GL_COPY, GL_AND_INVERTED, GL_NOOP, GL_XOR, GL_OR,
		GL_NOR, GL_EQUIV, GL_INVERT, GL_OR_REVERSE, GL_COPY_INVERTED, GL_OR_INVERTED, GL_NAND, GL_SET
	};
	const char *operationNames[] = {
		"clear", "and", "and-reverse", "copy", "and-inverted", "noop", "xor", "or",
		"nor", "equiv", "invert", "or-reverse", "copy-inverted", "or-inverted", "nand", "set"
	};
	const Rgba destination = { 60, 165, 90, 195 };
	const Rgba source = { 150, 105, 240, 15 };
	configure2D(64, 64);
	glClearColor(destination[0] / 255.0f, destination[1] / 255.0f,
		destination[2] / 255.0f, destination[3] / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_COLOR_LOGIC_OP);
	setColor(source);
	for (std::size_t i = 0; i < 16; i++)
	{
		const float left = 2.0f + static_cast<float>(i % 4) * 15.0f;
		const float bottom = 2.0f + static_cast<float>(i / 4) * 15.0f;
		glLogicOp(operations[i]);
		drawQuad(left, bottom, left + 10.0f, bottom + 10.0f, 0.0f);
	}
	glFinish();
	for (std::size_t i = 0; i < 16; i++)
	{
		const int x = 7 + static_cast<int>(i % 4) * 15;
		const int y = 7 + static_cast<int>(i / 4) * 15;
		const Rgba actual = readRgba(x, y);
		Rgba expected = {};
		for (std::size_t channel = 0; channel < 4; channel++)
			expected[channel] = logicResult(operations[i], source[channel], destination[channel]);
		const std::string name = "logic." + std::string(operationNames[i]);
		if (enforceExpected)
			requireExact(name, actual, expected);
		addRgbaCase(results, name, actual);
	}
}

void recordAsymmetricTransformCase(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(11, 17, 73, 61);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 2.0, -0.75, 1.25, 1.0, 9.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.6f, -0.3f, -3.0f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.7f, 1.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	drawQuad(-1.5f, -0.8f, 1.5f, 0.8f, 0.0f);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	drawQuad(-0.3f, -0.3f, 0.3f, 0.3f, 1.5f);
	glFinish();

	const int samples[][2] = {
		{ 29, 34 }, { 35, 29 }, { 35, 44 }, { 46, 43 },
		{ 44, 34 }, { 52, 34 }, { 40, 23 }, { 40, 52 }
	};
	std::vector<unsigned char> actual;
	for (const auto &sample : samples)
	{
		const Rgb pixel = readRgb(sample[0], sample[1]);
		actual.insert(actual.end(), pixel.begin(), pixel.end());
	}
	const std::vector<unsigned char> expected = {
		0, 0, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0,
		0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	const std::string name = "matrix.asymmetric-modelview-projection-viewport";
	if (enforceExpected)
		requireExact(name, actual, expected);
	addCase(results, name, byteHex(actual));
}

void recordTextureCases(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	const std::array<Rgba, 4> original = {
		Rgba{ 32, 64, 96, 255 }, Rgba{ 160, 32, 64, 255 },
		Rgba{ 16, 192, 48, 255 }, Rgba{ 224, 128, 16, 255 }
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, original.data());

	const std::vector<TextureCoordinate> edgeCoordinates = {
		{ -0.25f, 0.25f }, { 0.0f, 0.25f }, { 1.0f, 0.25f }, { 1.25f, 0.25f }
	};
	const char *edgeNames[] = { "u-neg025", "u-zero", "u-one", "u-125" };
	const Rgb nearestExpected[] = {
		{ 32, 64, 96 }, { 32, 64, 96 }, { 160, 32, 64 }, { 160, 32, 64 }
	};
	std::vector<Rgb> pixels = renderTextureSamples(edgeCoordinates);
	for (std::size_t i = 0; i < pixels.size(); i++)
	{
		const std::string name = "texture.nearest." + std::string(edgeNames[i]);
		if (enforceExpected)
			requireExact(name, pixels[i], nearestExpected[i]);
		addByteCase(results, name, pixels[i]);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	const Rgb linearExpected[] = {
		{ 16, 32, 48 }, { 16, 32, 48 }, { 80, 16, 32 }, { 80, 16, 32 }
	};
	pixels = renderTextureSamples(edgeCoordinates);
	for (std::size_t i = 0; i < pixels.size(); i++)
	{
		const std::string name = "texture.linear." + std::string(edgeNames[i]);
		if (enforceExpected)
			requireNear(name, pixels[i], linearExpected[i], 1);
		addByteCase(results, name, pixels[i]);
	}

	const std::array<Rgba, 4> updated = {
		Rgba{ 255, 0, 0, 255 }, Rgba{ 0, 255, 0, 255 },
		Rgba{ 0, 0, 255, 255 }, Rgba{ 255, 255, 0, 255 }
	};
	const int offsets[][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
	for (std::size_t i = 0; i < updated.size(); i++)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, offsets[i][0], offsets[i][1], 1, 1,
			GL_RGBA, GL_UNSIGNED_BYTE, updated[i].data());
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	const std::vector<TextureCoordinate> cornerCoordinates = {
		{ 0.25f, 0.25f }, { 0.75f, 0.25f }, { 0.25f, 0.75f }, { 0.75f, 0.75f }
	};
	const char *cornerNames[] = { "bottom-left", "bottom-right", "top-left", "top-right" };
	const Rgb cornerExpected[] = {
		{ 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }, { 255, 255, 0 }
	};
	pixels = renderTextureSamples(cornerCoordinates);
	for (std::size_t i = 0; i < pixels.size(); i++)
	{
		const std::string name = "texture.subimage." + std::string(cornerNames[i]);
		if (enforceExpected)
			requireExact(name, pixels[i], cornerExpected[i]);
		addByteCase(results, name, pixels[i]);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	const std::vector<TextureCoordinate> mixedCoordinates = {
		{ -0.25f, -0.25f }, { 1.25f, -0.25f }, { -0.25f, 1.25f }, { 1.25f, 1.25f }
	};
	const char *mixedNames[] = { "left-bottom", "right-bottom", "left-top", "right-top" };
	const Rgb mixedExpected[] = {
		{ 0, 0, 255 }, { 255, 255, 0 }, { 255, 0, 0 }, { 0, 255, 0 }
	};
	pixels = renderTextureSamples(mixedCoordinates);
	for (std::size_t i = 0; i < pixels.size(); i++)
	{
		const std::string name = "texture.mixed." + std::string(mixedNames[i]);
		if (enforceExpected)
			requireExact(name, pixels[i], mixedExpected[i]);
		addByteCase(results, name, pixels[i]);
	}

	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &texture);
}

void recordTextureZeroDeleteCase(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	glBindTexture(GL_TEXTURE_2D, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	const Rgba texel = { 37, 91, 173, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel.data());

	const std::vector<TextureCoordinate> coordinate = { { 0.5f, 0.5f } };
	const Rgb beforeDelete = renderTextureSamples(coordinate)[0];
	const GLuint zero = 0;
	glDeleteTextures(1, &zero);
	const Rgb afterDelete = renderTextureSamples(coordinate)[0];

	std::vector<unsigned char> actual;
	actual.insert(actual.end(), beforeDelete.begin(), beforeDelete.end());
	actual.insert(actual.end(), afterDelete.begin(), afterDelete.end());
	const std::vector<unsigned char> expected = { 37, 91, 173, 37, 91, 173 };
	const std::string name = "texture.zero-delete-preserves-default";
	if (enforceExpected)
		requireExact(name, actual, expected);
	addCase(results, name, byteHex(actual));
	glDisable(GL_TEXTURE_2D);
}

void recordDisplayListTextureCase(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	GLuint texture = 0;
	glGenTextures(1, &texture);
	const GLuint list = glGenLists(1);
	std::array<Rgba, 4> pixels = {
		Rgba{ 37, 91, 173, 255 }, Rgba{ 37, 91, 173, 255 },
		Rgba{ 37, 91, 173, 255 }, Rgba{ 37, 91, 173, 255 }
	};

	glNewList(list, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	drawTexturedQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.25f, 0.25f);
	glEndList();
	if (glGetError() != GL_NO_ERROR)
		throw std::runtime_error("case texture.display-list-first-use: compiling the list raised an error");

	for (Rgba &pixel : pixels)
		pixel = { 0, 0, 0, 0 };
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glCallList(list);
	if (glGetError() != GL_NO_ERROR)
		throw std::runtime_error("case texture.display-list-first-use: executing the list raised an error");
	glFinish();

	const Rgb result = readRgb(64, 64);
	if (enforceExpected)
		requireExact("texture.display-list-first-use", result, { 37, 91, 173 });
	addByteCase(results, "texture.display-list-first-use", result);

	glDisable(GL_TEXTURE_2D);
	glDeleteLists(list, 1);
	glDeleteTextures(1, &texture);
}

void recordDisplayListCurrentColorCase(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	const float vertices[] = {
		0.0f, 0.0f, 0.0f,
		16.0f, 0.0f, 0.0f,
		16.0f, 16.0f, 0.0f,
		0.0f, 16.0f, 0.0f
	};
	const GLuint list = glGenLists(1);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glNewList(list, GL_COMPILE);
	glDrawArrays(GL_QUADS, 0, 4);
	glEndList();
	glDisableClientState(GL_VERTEX_ARRAY);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glPushMatrix();
	glTranslatef(16.0f, 16.0f, 0.0f);
	glCallList(list);
	glPopMatrix();
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glPushMatrix();
	glTranslatef(48.0f, 16.0f, 0.0f);
	glCallList(list);
	glPopMatrix();
	glFinish();

	const Rgb first = readRgb(24, 24);
	const Rgb second = readRgb(56, 24);
	std::vector<unsigned char> actual;
	actual.insert(actual.end(), first.begin(), first.end());
	actual.insert(actual.end(), second.begin(), second.end());
	const std::vector<unsigned char> expected = { 255, 0, 0, 0, 255, 0 };
	const std::string name = "list.execution-current-color-variants";
	if (enforceExpected)
		requireExact(name, actual, expected);
	addCase(results, name, byteHex(actual));
	glDeleteLists(list, 1);
}

void recordPresentedTextureLifetimeCase(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	const std::array<Rgba, 4> frameColors = {
		Rgba{ 255, 0, 0, 255 }, Rgba{ 0, 255, 0, 255 },
		Rgba{ 0, 0, 255, 255 }, Rgba{ 37, 91, 173, 255 }
	};
	for (std::size_t frame = 0; frame < frameColors.size(); frame++)
	{
		std::array<Rgba, 4> pixels;
		pixels.fill(frameColors[frame]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		drawTexturedQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.25f, 0.25f);
		if (frame + 1 < frameColors.size())
		{
			// Three asynchronous presents wrap the two Vulkan frame slots before the final draw.
			renderbackend::present();
		}
	}

	const Rgb result = readRgb(64, 64);
	if (enforceExpected)
		requireExact("texture.present-redefine-slot-reuse", result, { 37, 91, 173 });
	addByteCase(results, "texture.present-redefine-slot-reuse", result);

	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &texture);
}

void recordTextureMatrixCases(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	const std::array<Rgba, 4> pixels = {
		Rgba{ 255, 0, 0, 255 }, Rgba{ 0, 255, 0, 255 },
		Rgba{ 0, 0, 255, 255 }, Rgba{ 255, 255, 0, 255 }
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	drawTexturedQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.25f, 0.25f);
	glFinish();
	const Rgb identity = readRgb(64, 64);
	if (enforceExpected)
		requireExact("texture-matrix.identity", identity, { 255, 0, 0 });
	addByteCase(results, "texture-matrix.identity", identity);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.5f, 0.0f, 0.0f);
	glScalef(0.5f, 1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT);
	drawTexturedQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.25f, 0.25f);
	glFinish();
	const Rgb transformed = readRgb(64, 64);
	if (enforceExpected)
		requireExact("texture-matrix.translate-scale", transformed, { 0, 255, 0 });
	addByteCase(results, "texture-matrix.translate-scale", transformed);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &texture);
}

Rgb renderPrimaryColor(GLenum shadeModel)
{
	configure2D(WindowWidth, WindowHeight);
	glShadeModel(shadeModel);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glVertex3f(16.0f, 16.0f, 0.0f);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glVertex3f(112.0f, 16.0f, 0.0f);
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	glVertex3f(16.0f, 112.0f, 0.0f);
	glEnd();
	glFinish();
	return readRgb(40, 40);
}

void recordPrimaryColorCases(ResultSet &results, bool enforceExpected)
{
	const Rgb flat = renderPrimaryColor(GL_FLAT);
	if (enforceExpected)
		requireExact("primary.flat.provoking", flat, { 0, 0, 255 });
	addByteCase(results, "primary.flat.provoking", flat);

	const Rgb smooth = renderPrimaryColor(GL_SMOOTH);
	if (enforceExpected)
		requireNear("primary.smooth.interior", smooth, { 125, 65, 65 }, 2);
	addByteCase(results, "primary.smooth.interior", smooth);
}

Rgb renderInterleavedArray(bool useBuffer)
{
	configure2D(WindowWidth, WindowHeight);
	const std::array<InterleavedVertex, 3> vertices = {
		InterleavedVertex{ 16.0f, 16.0f, 0.0f, 0.0f, 0.0f, 37, 91, 173, 255, 0, 0, 127, 0, 0 },
		InterleavedVertex{ 112.0f, 16.0f, 0.0f, 0.0f, 0.0f, 37, 91, 173, 255, 0, 0, 127, 0, 0 },
		InterleavedVertex{ 16.0f, 112.0f, 0.0f, 0.0f, 0.0f, 37, 91, 173, 255, 0, 0, 127, 0, 0 }
	};

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	if (useBuffer)
	{
		GLuint buffer = 0;
		glGenBuffersARB(1, &buffer);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(sizeof(vertices)), vertices.data(),
			GL_STREAM_DRAW_ARB);
	}

	const GLvoid *position = useBuffer ? reinterpret_cast<const GLvoid *>(0) : &vertices[0].x;
	const GLvoid *color = useBuffer ? reinterpret_cast<const GLvoid *>(offsetof(InterleavedVertex, r)) : &vertices[0].r;
	glVertexPointer(3, GL_FLOAT, static_cast<GLsizei>(sizeof(InterleavedVertex)), position);
	glColorPointer(4, GL_UNSIGNED_BYTE, static_cast<GLsizei>(sizeof(InterleavedVertex)), color);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glFinish();
	return readRgb(40, 40);
}

void recordInterleavedArrayCases(ResultSet &results, bool enforceExpected)
{
	const Rgb client = renderInterleavedArray(false);
	if (enforceExpected)
		requireExact("arrays.interleaved32.client-pointer", client, { 37, 91, 173 });
	addByteCase(results, "arrays.interleaved32.client-pointer", client);

	const Rgb buffer = renderInterleavedArray(true);
	if (enforceExpected)
		requireExact("arrays.interleaved32.buffer-offset", buffer, { 37, 91, 173 });
	addByteCase(results, "arrays.interleaved32.buffer-offset", buffer);
}

void setDirectionalLight(GLenum light, const GLfloat *direction, const GLfloat *diffuse)
{
	const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightfv(light, GL_AMBIENT, black);
	glLightfv(light, GL_DIFFUSE, diffuse);
	glLightfv(light, GL_SPECULAR, black);
	glLightfv(light, GL_POSITION, direction);
}

void configureColorMaterialLighting()
{
	const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
}

void recordTwoLightCase(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	const GLfloat light0Direction[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	const GLfloat light1Direction[] = { 0.0f, 1.0f, 1.0f, 0.0f };
	const GLfloat light0Diffuse[] = { 0.5f, 0.0f, 0.0f, 1.0f };
	const GLfloat light1Diffuse[] = { 0.0f, 0.5f, 0.0f, 1.0f };
	setDirectionalLight(GL_LIGHT0, light0Direction, light0Diffuse);
	setDirectionalLight(GL_LIGHT1, light1Direction, light1Diffuse);
	configureColorMaterialLighting();
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor4f(0.5f, 0.75f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	drawQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.0f);
	glFinish();
	const Rgb pixel = readRgb(64, 64);
	if (enforceExpected)
		requireNear("lighting.two-directional-color-material", pixel, { 64, 68, 0 }, 2);
	addByteCase(results, "lighting.two-directional-color-material", pixel);
}

Rgb renderNormalMode(GLenum mode)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_RESCALE_NORMAL);
	glDisable(GL_NORMALIZE);
	if (mode != 0)
		glEnable(mode);
	glScalef(2.0f, 2.0f, 2.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 0.5f);
	drawQuad(24.0f, 24.0f, 40.0f, 40.0f, 0.0f);
	glFinish();
	return readRgb(64, 64);
}

void recordNormalModeCases(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	const GLfloat direction[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	const GLfloat diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	setDirectionalLight(GL_LIGHT0, direction, diffuse);
	configureColorMaterialLighting();
	glEnable(GL_LIGHT0);

	const Rgb none = renderNormalMode(0);
	const Rgb rescale = renderNormalMode(GL_RESCALE_NORMAL);
	const Rgb normalize = renderNormalMode(GL_NORMALIZE);
	if (enforceExpected)
	{
		requireNear("normal.none.scaled", none, { 32, 32, 32 }, 2);
		requireNear("normal.rescale.scaled", rescale, { 64, 64, 64 }, 2);
		requireNear("normal.normalize.scaled", normalize, { 128, 128, 128 }, 2);
	}
	addByteCase(results, "normal.none.scaled", none);
	addByteCase(results, "normal.rescale.scaled", rescale);
	addByteCase(results, "normal.normalize.scaled", normalize);
}

Rgb renderFogMode(GLenum distanceMode)
{
	glFogi(GL_FOG_DISTANCE_MODE_NV, static_cast<GLint>(distanceMode));
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	drawQuad(2.5f, -0.5f, 3.5f, 0.5f, -4.0f);
	glFinish();
	return readRgb(88, 64);
}

void recordFogCases(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-8.0, 8.0, -8.0, 8.0, -10.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glFogfv(GL_FOG_COLOR, black);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0.0f);
	glFogf(GL_FOG_END, 10.0f);
	glEnable(GL_FOG);

	const Rgb absolute = renderFogMode(GL_EYE_PLANE_ABSOLUTE_NV);
	const Rgb plane = renderFogMode(GL_EYE_PLANE);
	const Rgb radial = renderFogMode(GL_EYE_RADIAL_NV);
	glFogi(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, 0.25f);
	const Rgb exponential = renderFogMode(GL_EYE_PLANE_ABSOLUTE_NV);
	if (enforceExpected)
	{
		if (std::abs(static_cast<int>(radial[0]) - static_cast<int>(absolute[0])) < 8)
			throw std::runtime_error("case fog.eye-radial: expected radial distance to differ from absolute eye Z");
		requireNear("fog.exp.eye-plane-absolute", exponential, { 94, 94, 94 }, 2);
	}
	addByteCase(results, "fog.eye-plane-absolute", absolute);
	addByteCase(results, "fog.eye-plane-signed", plane);
	addByteCase(results, "fog.eye-radial", radial);
	addByteCase(results, "fog.exp.eye-plane-absolute", exponential);
}

void recordClearCases(ResultSet &results, bool enforceExpected)
{
	configure2D(WindowWidth, WindowHeight);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
	glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFinish();
	const Rgb maskedColor = readRgb(64, 64);
	if (enforceExpected)
		requireExact("clear.color-mask", maskedColor, { 255, 255, 255 });
	addByteCase(results, "clear.color-mask", maskedColor);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glClearDepth(0.25);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	drawQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.0f);
	glFinish();
	const Rgb maskedDepth = readRgb(64, 64);

	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_TRUE);
	glClearDepth(0.25);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	drawQuad(48.0f, 48.0f, 80.0f, 80.0f, 0.0f);
	glFinish();
	const Rgb unmaskedDepth = readRgb(64, 64);
	const bool firstPassed = maskedDepth[0] == 255 && maskedDepth[1] == 0 && maskedDepth[2] == 0;
	const bool secondFailed = unmaskedDepth[0] == 0 && unmaskedDepth[1] == 0 && unmaskedDepth[2] == 0;
	if (enforceExpected && (!firstPassed || !secondFailed))
		throw std::runtime_error("case clear.depth-mask: expected visibility signature 10");
	addCase(results, "clear.depth-mask", "10");

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClearDepth(1.0);
}

std::size_t alignedStride(std::size_t rowBytes, int alignment)
{
	return (rowBytes + static_cast<std::size_t>(alignment) - 1) & ~(static_cast<std::size_t>(alignment) - 1);
}

std::vector<unsigned char> expectedReadback(GLenum format, int alignment)
{
	const Rgb colors[2][3] = {
		{ Rgb{ 255, 0, 0 }, Rgb{ 0, 255, 0 }, Rgb{ 0, 0, 255 } },
		{ Rgb{ 255, 255, 0 }, Rgb{ 255, 0, 255 }, Rgb{ 0, 255, 255 } }
	};
	const std::size_t components = format == GL_RGBA ? 4 : 3;
	const std::size_t rowBytes = 3 * components;
	const std::size_t stride = alignedStride(rowBytes, alignment);
	std::vector<unsigned char> result(stride * 2, 0xCD);
	for (std::size_t y = 0; y < 2; y++)
	{
		for (std::size_t x = 0; x < 3; x++)
		{
			unsigned char *destination = result.data() + y * stride + x * components;
			if (format == GL_BGR_EXT)
			{
				destination[0] = colors[y][x][2];
				destination[1] = colors[y][x][1];
				destination[2] = colors[y][x][0];
			}
			else
			{
				destination[0] = colors[y][x][0];
				destination[1] = colors[y][x][1];
				destination[2] = colors[y][x][2];
				if (format == GL_RGBA)
					destination[3] = 255;
			}
		}
	}
	return result;
}

void recordReadbackCases(ResultSet &results, bool enforceExpected)
{
	configure2D(3, 2);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	const Rgb colors[2][3] = {
		{ Rgb{ 255, 0, 0 }, Rgb{ 0, 255, 0 }, Rgb{ 0, 0, 255 } },
		{ Rgb{ 255, 255, 0 }, Rgb{ 255, 0, 255 }, Rgb{ 0, 255, 255 } }
	};
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			glColor4f(colors[y][x][0] / 255.0f, colors[y][x][1] / 255.0f,
				colors[y][x][2] / 255.0f, 1.0f);
			drawQuad(static_cast<float>(x), static_cast<float>(y),
				static_cast<float>(x + 1), static_cast<float>(y + 1), 0.0f);
		}
	}
	glFinish();

	const GLenum formats[] = { GL_RGB, GL_BGR_EXT, GL_RGBA };
	const char *formatNames[] = { "rgb", "bgr", "rgba" };
	const int alignments[] = { 1, 2, 4, 8 };
	for (std::size_t formatIndex = 0; formatIndex < 3; formatIndex++)
	{
		for (int alignment : alignments)
		{
			const std::size_t components = formats[formatIndex] == GL_RGBA ? 4 : 3;
			const std::size_t stride = alignedStride(3 * components, alignment);
			std::vector<unsigned char> actual(stride * 2, 0xCD);
			glPixelStorei(GL_PACK_ALIGNMENT, alignment);
			glReadPixels(0, 0, 3, 2, formats[formatIndex], GL_UNSIGNED_BYTE, actual.data());
			const std::vector<unsigned char> expected = expectedReadback(formats[formatIndex], alignment);
			const std::string name = "readback." + std::string(formatNames[formatIndex]) + ".pack" + std::to_string(alignment);
			if (enforceExpected && actual != expected)
				throw std::runtime_error("case " + name + ": expected " + byteHex(expected) + ", got " + byteHex(actual));
			addCase(results, name, byteHex(actual));
		}
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

void drawSlopedTriangle()
{
	glBegin(GL_TRIANGLES);
	glVertex3f(4.0f, 4.0f, -0.5f);
	glVertex3f(28.0f, 4.0f, 0.5f);
	glVertex3f(4.0f, 28.0f, -0.5f);
	glEnd();
}

bool renderPolygonOffset(float factor, float units, bool sloped)
{
	configure2D(32, 32);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	if (sloped)
		drawSlopedTriangle();
	else
		drawQuad(4.0f, 4.0f, 28.0f, 28.0f, 0.0f);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(factor, units);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	if (sloped)
		drawSlopedTriangle();
	else
		drawQuad(4.0f, 4.0f, 28.0f, 28.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glFinish();
	const Rgb pixel = sloped ? readRgb(10, 10) : readRgb(16, 16);
	return pixel[1] > pixel[0];
}

void recordPolygonOffsetCases(ResultSet &results)
{
	const float units[] = { -4.0f, -1.0f, -0.25f, 0.0f, 0.25f, 1.0f, 4.0f };
	const char *unitNames[] = { "neg4", "neg1", "neg025", "zero", "pos025", "pos1", "pos4" };
	for (std::size_t i = 0; i < 7; i++)
	{
		const bool visible = renderPolygonOffset(0.0f, units[i], false);
		addCase(results, "polygon.flat.units." + std::string(unitNames[i]), visible ? "1" : "0");
	}

	const float factors[] = { -1.0f, 0.0f, 1.0f };
	const char *factorNames[] = { "neg1", "zero", "pos1" };
	for (std::size_t i = 0; i < 3; i++)
	{
		const bool visible = renderPolygonOffset(factors[i], 0.0f, true);
		addCase(results, "polygon.slope.factor." + std::string(factorNames[i]), visible ? "1" : "0");
	}
}

std::vector<unsigned char> renderLineMask(float width, bool diagonal)
{
	configure2D(LineMaskWidth, LineMaskHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glLineWidth(width);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	if (diagonal)
	{
		glVertex3f(4.0f, 4.0f, 0.0f);
		glVertex3f(28.0f, 28.0f, 0.0f);
	}
	else
	{
		glVertex3f(4.0f, 16.0f, 0.0f);
		glVertex3f(28.0f, 16.0f, 0.0f);
	}
	glEnd();
	glFinish();

	std::vector<unsigned char> pixels(LineMaskWidth * LineMaskHeight * 3, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, LineMaskWidth, LineMaskHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
	std::vector<unsigned char> mask((LineMaskWidth * LineMaskHeight + 7) / 8, 0);
	for (int i = 0; i < LineMaskWidth * LineMaskHeight; i++)
	{
		if (pixels[i * 3] != 0 || pixels[i * 3 + 1] != 0 || pixels[i * 3 + 2] != 0)
			mask[static_cast<std::size_t>(i) / 8] |= static_cast<unsigned char>(1U << (i & 7));
	}
	return mask;
}

void recordLineCases(ResultSet &results)
{
	addCase(results, "line.width1.horizontal", byteHex(renderLineMask(1.0f, false)));
	addCase(results, "line.width1.diagonal", byteHex(renderLineMask(1.0f, true)));
	addCase(results, "line.width2.horizontal", byteHex(renderLineMask(2.0f, false)));
	addCase(results, "line.width2.diagonal", byteHex(renderLineMask(2.0f, true)));
}

ResultSet recordCases()
{
	platform::initialize();
	if (SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8) != 0)
		throw std::runtime_error("legacygl GPU parity fixture could not request an alpha framebuffer");
	lwjgl::GLContext::instantiate();
	platform::setWindowSize(WindowWidth, WindowHeight);
	int drawableWidth = 0;
	int drawableHeight = 0;
	platform::getDrawableSize(drawableWidth, drawableHeight);
	if (drawableWidth < WindowWidth || drawableHeight < WindowHeight)
		throw std::runtime_error("legacygl GPU parity fixture requires a 128x128 drawable");

	ResultSet results;
	results.backend = renderbackend::configuration().recordName;
	const bool enforceExpected = results.backend == "native";
	recordGeneratedNameCases(results, enforceExpected);
	recordAlphaCases(results, enforceExpected);
	recordCullCases(results, enforceExpected);
	recordDepthCases(results, enforceExpected);
	recordBlendCases(results);
	recordLogicOpCases(results, enforceExpected);
	recordAsymmetricTransformCase(results, enforceExpected);
	recordTextureCases(results, enforceExpected);
	recordTextureZeroDeleteCase(results, enforceExpected);
	recordDisplayListTextureCase(results, enforceExpected);
	recordDisplayListCurrentColorCase(results, enforceExpected);
	recordPresentedTextureLifetimeCase(results, enforceExpected);
	recordTextureMatrixCases(results, enforceExpected);
	recordPrimaryColorCases(results, enforceExpected);
	recordInterleavedArrayCases(results, enforceExpected);
	recordTwoLightCase(results, enforceExpected);
	recordNormalModeCases(results, enforceExpected);
	recordFogCases(results, enforceExpected);
	recordClearCases(results, enforceExpected);
	recordReadbackCases(results, enforceExpected);
	recordPolygonOffsetCases(results);
	recordLineCases(results);
	return results;
}

void writeResults(const std::string &path, const ResultSet &results)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	if (!output)
		throw std::runtime_error("cannot write " + path);
	output << RecordHeader << '\n';
	output << "backend " << results.backend << '\n';
	for (const CaseResult &result : results.cases)
		output << "case " << result.name << ' ' << result.value << '\n';
	if (!output)
		throw std::runtime_error("failed while writing " + path);
}

ResultSet readResults(const std::string &path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
		throw std::runtime_error("cannot read " + path);
	std::string line;
	if (!std::getline(input, line) || line != RecordHeader)
		throw std::runtime_error(path + ": unsupported or missing record header");

	ResultSet results;
	int lineNumber = 1;
	while (std::getline(input, line))
	{
		lineNumber++;
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;
		std::istringstream fields(line);
		std::string kind;
		std::string name;
		std::string value;
		std::string extra;
		fields >> kind;
		if (kind == "backend")
		{
			if (!(fields >> value) || (fields >> extra) || !results.backend.empty())
				throw std::runtime_error(path + ": malformed backend on line " + std::to_string(lineNumber));
			results.backend = value;
		}
		else if (kind == "case")
		{
			if (!(fields >> name >> value) || (fields >> extra))
				throw std::runtime_error(path + ": malformed case on line " + std::to_string(lineNumber));
			addCase(results, name, value);
		}
		else
		{
			throw std::runtime_error(path + ": unknown record on line " + std::to_string(lineNumber));
		}
	}
	if (results.backend.empty())
		throw std::runtime_error(path + ": missing backend record");
	return results;
}

const CaseResult &findCase(const ResultSet &results, const std::string &name)
{
	for (const CaseResult &result : results.cases)
	{
		if (result.name == name)
			return result;
	}
	throw std::runtime_error("case " + name + ": missing from " + results.backend + " record");
}

bool onePixelBoundary(const std::vector<unsigned char> &first, const std::vector<unsigned char> &second)
{
	if (first.size() != second.size() || first.size() * 8 < LineMaskWidth * LineMaskHeight)
		return false;
	const auto set = [](const std::vector<unsigned char> &mask, int x, int y)
	{
		if (x < 0 || x >= LineMaskWidth || y < 0 || y >= LineMaskHeight)
			return false;
		const int index = y * LineMaskWidth + x;
		return (mask[static_cast<std::size_t>(index) / 8] & static_cast<unsigned char>(1U << (index & 7))) != 0;
	};
	int firstCount = 0;
	int secondCount = 0;
	int firstBounds[] = { LineMaskWidth, LineMaskHeight, -1, -1 };
	int secondBounds[] = { LineMaskWidth, LineMaskHeight, -1, -1 };
	for (int y = 0; y < LineMaskHeight; y++)
	{
		for (int x = 0; x < LineMaskWidth; x++)
		{
			if (set(first, x, y))
			{
				firstCount++;
				firstBounds[0] = std::min(firstBounds[0], x);
				firstBounds[1] = std::min(firstBounds[1], y);
				firstBounds[2] = std::max(firstBounds[2], x);
				firstBounds[3] = std::max(firstBounds[3], y);
			}
			if (set(second, x, y))
			{
				secondCount++;
				secondBounds[0] = std::min(secondBounds[0], x);
				secondBounds[1] = std::min(secondBounds[1], y);
				secondBounds[2] = std::max(secondBounds[2], x);
				secondBounds[3] = std::max(secondBounds[3], y);
			}
		}
	}
	if (firstCount == 0 || secondCount == 0 ||
		std::max(firstCount, secondCount) > std::min(firstCount, secondCount) * 2 + 4)
		return false;
	for (int i = 0; i < 4; i++)
	{
		if (std::abs(firstBounds[i] - secondBounds[i]) > 1)
			return false;
	}

	bool different = false;
	for (int y = 0; y < LineMaskHeight; y++)
	{
		for (int x = 0; x < LineMaskWidth; x++)
		{
			const bool firstPixel = set(first, x, y);
			const bool secondPixel = set(second, x, y);
			if (firstPixel == secondPixel)
				continue;
			different = true;
			bool neighbor = false;
			for (int dy = -1; dy <= 1; dy++)
			{
				for (int dx = -1; dx <= 1; dx++)
				{
					if (firstPixel)
						neighbor = neighbor || set(second, x + dx, y + dy);
					else
						neighbor = neighbor || set(first, x + dx, y + dy);
				}
			}
			if (!neighbor)
				return false;
		}
	}
	return different;
}

void compareExactCase(const CaseResult &reference, const CaseResult &candidate,
	const std::string &referenceName, const std::string &candidateName)
{
	if (reference.value != candidate.value)
		throw std::runtime_error("case " + reference.name + ": " + referenceName + "=" +
			reference.value + ", " +
			candidateName + "=" + candidate.value);
}

void compareTolerantCase(const CaseResult &reference, const CaseResult &candidate, int tolerance,
	const std::string &referenceName, const std::string &candidateName)
{
	const std::vector<unsigned char> referenceBytes = parseHex(reference.value, reference.name);
	const std::vector<unsigned char> candidateBytes = parseHex(candidate.value, candidate.name);
	if (referenceBytes.size() != candidateBytes.size())
		throw std::runtime_error("case " + reference.name + ": byte count differs");
	for (std::size_t i = 0; i < referenceBytes.size(); i++)
	{
		if (std::abs(static_cast<int>(referenceBytes[i]) - static_cast<int>(candidateBytes[i])) > tolerance)
			throw std::runtime_error("case " + reference.name + ": channel " + std::to_string(i) +
				" " + referenceName + "=" + std::to_string(referenceBytes[i]) + ", " + candidateName + "=" +
				std::to_string(candidateBytes[i]));
	}
}

void compareLineCase(const ResultSet &nativeResults, const ResultSet &candidateResults, const std::string &shape)
{
	const CaseResult &nativeWidth2 = findCase(nativeResults, "line.width2." + shape);
	const CaseResult &candidateWidth2 = findCase(candidateResults, "line.width2." + shape);
	if (nativeWidth2.value == candidateWidth2.value)
		return;

	const CaseResult &nativeWidth1 = findCase(nativeResults, "line.width1." + shape);
	const CaseResult &candidateWidth1 = findCase(candidateResults, "line.width1." + shape);
	if (candidateWidth2.value == candidateWidth1.value && nativeWidth2.value != nativeWidth1.value)
	{
		std::cout << "legacygl-gpu-parity: " << nativeWidth2.name << " classified width-1 fallback\n";
		return;
	}

	const std::vector<unsigned char> nativeMask = parseHex(nativeWidth2.value, nativeWidth2.name);
	const std::vector<unsigned char> candidateMask = parseHex(candidateWidth2.value, candidateWidth2.name);
	if (onePixelBoundary(nativeMask, candidateMask))
	{
		std::cout << "legacygl-gpu-parity: " << nativeWidth2.name << " classified one-pixel boundary\n";
		return;
	}
	throw std::runtime_error("case " + nativeWidth2.name + ": line masks differ beyond classification");
}

void compareResults(const ResultSet &referenceResults, const ResultSet &candidateResults)
{
	if (referenceResults.backend == candidateResults.backend)
		throw std::runtime_error("cannot compare two " + referenceResults.backend + " records");
	if (referenceResults.cases.size() != candidateResults.cases.size())
		throw std::runtime_error("record case counts differ: " + referenceResults.backend + "=" +
			std::to_string(referenceResults.cases.size()) +
			", " + candidateResults.backend + "=" + std::to_string(candidateResults.cases.size()));

	for (const CaseResult &reference : referenceResults.cases)
	{
		const CaseResult &candidate = findCase(candidateResults, reference.name);
		if (reference.name == "line.width2.horizontal" || reference.name == "line.width2.diagonal")
			continue;
		if (reference.name.compare(0, 15, "texture.linear.") == 0)
			compareTolerantCase(reference, candidate, 1, referenceResults.backend, candidateResults.backend);
		else if (reference.name == "primary.smooth.interior" ||
			reference.name.compare(0, 9, "lighting.") == 0 ||
			reference.name.compare(0, 7, "normal.") == 0 ||
			reference.name.compare(0, 4, "fog.") == 0)
			compareTolerantCase(reference, candidate, 2, referenceResults.backend, candidateResults.backend);
		else
			compareExactCase(reference, candidate, referenceResults.backend, candidateResults.backend);
	}
	compareLineCase(referenceResults, candidateResults, "horizontal");
	compareLineCase(referenceResults, candidateResults, "diagonal");
	std::cout << "legacygl-gpu-parity: compared " << referenceResults.cases.size() << " named cases\n";
}

void printUsage()
{
	std::cerr << "Usage:\n"
		"  a126cpp-legacygl-gpu-parity --record <path>\n"
		"  a126cpp-legacygl-gpu-parity --compare <native> <candidate>\n";
}

int run(int argc, char **argv)
{
	if (argc == 3 && std::string(argv[1]) == "--record")
	{
		const ResultSet results = recordCases();
		writeResults(argv[2], results);
		std::cout << "legacygl-gpu-parity: recorded " << results.cases.size() << " named cases to " << argv[2] << '\n';
		return 0;
	}
	if (argc == 4 && std::string(argv[1]) == "--compare")
	{
		compareResults(readResults(argv[2]), readResults(argv[3]));
		return 0;
	}
	printUsage();
	return 2;
}

}

int main(int argc, char **argv)
{
	try
	{
		return gpuparity::run(argc, argv);
	}
	catch (const std::exception &error)
	{
		std::cerr << "legacygl-gpu-parity: " << error.what() << '\n';
		return 1;
	}
}
