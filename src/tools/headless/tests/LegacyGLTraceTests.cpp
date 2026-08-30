// LegacyGL deterministic trace tests.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "legacygl/Trace.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/tests/LegacyGLFixture.h"

HEADLESS_TEST(legacygl_trace, capture_frame_is_deferred_and_renumbered)
{
	legacygl::Context &gl = legacyglTest::begin();
	const char *path = "legacygl-capture-frame-test.trace";
	std::remove(path);

	legacygl::traceCaptureFrameOnly();
	legacygl::traceOpen(path);
	ctx.check(!legacygl::traceEnabled(), "capture trace is deferred after opening");

	glColor3f(0.0f, 0.0f, 0.0f);
	legacygl::traceBeginCaptureFrame(gl.sequence());
	ctx.check(legacygl::traceEnabled(), "capture trace begins for the requested frame");
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glMatrixMode(0xDEAD);
	legacygl::traceEndCaptureFrame();
	ctx.check(!legacygl::traceEnabled(), "capture trace ends after the requested frame");

	glGetError();
	glColor3f(0.0f, 1.0f, 0.0f);

	std::ifstream input(path, std::ios::in | std::ios::binary);
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		lines.push_back(line);
	}
	input.close();
	std::remove(path);

	ctx.checkEqual(static_cast<long long>(lines.size()), 4LL,
		"capture trace contains only its header and final-frame calls");
	if (lines.size() == 4)
	{
		ctx.checkEqual(lines[0], "# a126cpp LegacyGL frontend trace", "trace header");
		ctx.checkEqual(lines[1], "1 glColor4f(1, 0, 0, 1)", "first frame call is numbered one");
		ctx.checkEqual(lines[2], "2 glMatrixMode(57005)", "frame call numbers are contiguous");
		ctx.checkEqual(lines[3], "# error 0x500 at call 2", "trace comments use relative numbering");
	}
}

HEADLESS_TEST(legacygl_trace, texture_hash_excludes_padding_after_the_final_row)
{
	legacyglTest::begin();
	const char *path = "legacygl-texture-footprint-test.trace";
	std::remove(path);

	unsigned char pixels[32];
	for (std::size_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = static_cast<unsigned char>(i + 1);
	const unsigned long long expected = legacygl::traceHash(pixels, 25);
	const unsigned long long paddedFinalRow = legacygl::traceHash(pixels, sizeof(pixels));

	legacygl::traceOpen(path);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 3, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	legacygl::traceClose();

	std::ifstream input(path, std::ios::in | std::ios::binary);
	std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	input.close();
	std::remove(path);

	ctx.check(contents.find(std::to_string(expected)) != std::string::npos,
		"the texture trace hashes the two rows and only their required inter-row padding");
	ctx.check(contents.find(std::to_string(paddedFinalRow)) == std::string::npos,
		"the texture trace does not read padding after the final row");
}
