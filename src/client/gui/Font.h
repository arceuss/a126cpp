#pragma once

#include <array>
#include <string>
#include <vector>

#include "client/MemoryTracker.h"

#include "java/Type.h"
#include "java/String.h"

class Options;
class Textures;
class BufferedImage;

class Font
{
private:
	std::array<int_t, 256> charWidths;
	// Alpha: FontRenderer.field_22009_h - packed RGB per colour code, with the
	// darkened shadow variants in entries 16-31 (FontRenderer.java:91-113).
	std::array<int_t, 32> colorCodeRGB;
public:
	int_t fontTexture = 0;

	Font(Options &options, const jstring &name, Textures &textures);

private:
	int_t listPos = 0;
	std::vector<int_t> ib = MemoryTracker::createIntBuffer(1024);

public:
	void drawShadow(const jstring &str, int_t x, int_t y, int_t color);
	void draw(const jstring &str, int_t x, int_t y, int_t color);
	void draw(const jstring &str, int_t x, int_t y, int_t color, bool darken);

	int_t width(const jstring &str);
	jstring trimStringToWidth(const jstring &str, int_t width, bool reverse = false);

	// One draw call for a sign's four lines. Same glyph quads, advances and
	// colour codes as `draw`, emitted through the Tesselator in one batch
	// instead of one display-list call per glyph. Signs are the only place that
	// draws thousands of short strings per frame, and the per-glyph submissions
	// were the frame's dominant cost.
	void drawLinesBatched(const jstring *lines, const int_t *xs, const int_t *ys, int_t lineCount, int_t color);

	// Pixel-equivalent sign text emitter for an enclosing OpenGL display-list
	// compilation.  It writes the exact triangle order, UVs, advances and
	// per-glyph colours used by drawLinesBatched, without constructing or
	// streaming a Tesselator buffer.
	void drawLinesImmediate(const jstring *lines, const int_t *xs, const int_t *ys, int_t lineCount, int_t color);

	// Alpha: FontRenderer.init scans each glyph cell for its last used column
	// (FontRenderer.java:62-89).
	static std::array<int_t, 256> computeCharWidths(const BufferedImage &image);

	// Alpha: FontRenderer.getStringWidth - complete colour codes consume the
	// following character and add no width; a dangling section sign contributes
	// negative one.
	static int_t widthOf(const std::array<int_t, 256> &charWidths, const jstring &str);

	static jstring sanitize(const jstring &str);
};
