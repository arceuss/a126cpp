#pragma once

#include <array>
#include <string>

#include "java/Type.h"
#include "java/String.h"

class Options;
class Textures;
class BufferedImage;
class Tesselator;
struct ModelMatrix;

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

public:
	void drawShadow(const jstring &str, int_t x, int_t y, int_t color);
	void draw(const jstring &str, int_t x, int_t y, int_t color);
	void draw(const jstring &str, int_t x, int_t y, int_t color, bool darken);

	int_t width(const jstring &str);
	jstring trimStringToWidth(const jstring &str, int_t width, bool reverse = false);

	// One draw call for several lines of text. Same glyph quads, advances and
	// colour codes Alpha's per-glyph display lists carried, emitted through
	// the Tesselator in one batch. `draw` is this with one line; signs pass
	// their four lines at once.
	void drawLinesBatched(const jstring *lines, const int_t *xs, const int_t *ys, int_t lineCount,
		int_t color, bool darken = false);
	// The same glyphs appended to a caller's open Tesselator batch, each
	// vertex taken through `transform` when one is given, so text from many
	// objects can share one draw. The caller binds the font texture.
	void appendLines(Tesselator &t, const jstring *lines, const int_t *xs, const int_t *ys,
		int_t lineCount, int_t color, bool darken, const ModelMatrix *transform);
	void bindFontTexture();

	// Alpha: FontRenderer.init scans each glyph cell for its last used column
	// (FontRenderer.java:62-89).
	static std::array<int_t, 256> computeCharWidths(const BufferedImage &image);

	// Alpha: FontRenderer.getStringWidth - complete colour codes consume the
	// following character and add no width; a dangling section sign contributes
	// negative one.
	static int_t widthOf(const std::array<int_t, 256> &charWidths, const jstring &str);

	static jstring sanitize(const jstring &str);
};
