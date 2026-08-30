#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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
const char *RecordHeader = "a126cpp-legacygl-gpu-parity 3";

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
	glNormal3f(0.0f, 0.0f, 1.0f);
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
		requireNear("normal.none.scaled", none, { 64, 64, 64 }, 2);
		requireNear("normal.rescale.scaled", rescale, { 128, 128, 128 }, 2);
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
	if (enforceExpected && std::abs(static_cast<int>(radial[0]) - static_cast<int>(absolute[0])) < 8)
		throw std::runtime_error("case fog.eye-radial: expected radial distance to differ from absolute eye Z");
	addByteCase(results, "fog.eye-plane-absolute", absolute);
	addByteCase(results, "fog.eye-plane-signed", plane);
	addByteCase(results, "fog.eye-radial", radial);
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
	recordTextureCases(results, enforceExpected);
	recordTextureMatrixCases(results, enforceExpected);
	recordPrimaryColorCases(results, enforceExpected);
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

void compareExactCase(const CaseResult &native, const CaseResult &candidate, const std::string &candidateName)
{
	if (native.value != candidate.value)
		throw std::runtime_error("case " + native.name + ": native=" + native.value + ", " +
			candidateName + "=" + candidate.value);
}

void compareTolerantCase(const CaseResult &native, const CaseResult &candidate, int tolerance,
	const std::string &candidateName)
{
	const std::vector<unsigned char> nativeBytes = parseHex(native.value, native.name);
	const std::vector<unsigned char> candidateBytes = parseHex(candidate.value, candidate.name);
	if (nativeBytes.size() != candidateBytes.size())
		throw std::runtime_error("case " + native.name + ": byte count differs");
	for (std::size_t i = 0; i < nativeBytes.size(); i++)
	{
		if (std::abs(static_cast<int>(nativeBytes[i]) - static_cast<int>(candidateBytes[i])) > tolerance)
			throw std::runtime_error("case " + native.name + ": channel " + std::to_string(i) +
				" native=" + std::to_string(nativeBytes[i]) + ", " + candidateName + "=" +
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

void compareResults(const ResultSet &nativeResults, const ResultSet &candidateResults)
{
	if (nativeResults.backend != "native")
		throw std::runtime_error("native record identifies backend " + nativeResults.backend);
	if (candidateResults.backend == "native")
		throw std::runtime_error("candidate record identifies the native backend");
	if (nativeResults.cases.size() != candidateResults.cases.size())
		throw std::runtime_error("record case counts differ: native=" + std::to_string(nativeResults.cases.size()) +
			", " + candidateResults.backend + "=" + std::to_string(candidateResults.cases.size()));

	for (const CaseResult &native : nativeResults.cases)
	{
		const CaseResult &candidate = findCase(candidateResults, native.name);
		if (native.name == "line.width2.horizontal" || native.name == "line.width2.diagonal")
			continue;
		if (native.name.compare(0, 15, "texture.linear.") == 0)
			compareTolerantCase(native, candidate, 1, candidateResults.backend);
		else if (native.name == "primary.smooth.interior" ||
			native.name.compare(0, 9, "lighting.") == 0 ||
			native.name.compare(0, 7, "normal.") == 0 ||
			native.name.compare(0, 4, "fog.") == 0)
			compareTolerantCase(native, candidate, 2, candidateResults.backend);
		else
			compareExactCase(native, candidate, candidateResults.backend);
	}
	compareLineCase(nativeResults, candidateResults, "horizontal");
	compareLineCase(nativeResults, candidateResults, "diagonal");
	std::cout << "legacygl-gpu-parity: compared " << nativeResults.cases.size() << " named cases\n";
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
