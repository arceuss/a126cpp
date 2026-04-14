#pragma once

#include <array>
#include <string>
#include <vector>

#include "client/MemoryTracker.h"

#include "java/Type.h"
#include "java/String.h"

class Options;
class Textures;

class Font
{
private:
	std::array<int_t, 256> charWidths;
	// Pre-calculated color code RGB values (0-15 = normal, 16-31 = darkened/shadow)
	std::array<float, 32> colorCodeR;
	std::array<float, 32> colorCodeG;
	std::array<float, 32> colorCodeB;
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

	// Optimized batch rendering for signs - renders all text in a single draw call
	void drawSignTextBatched(const jstring lines[4], const int_t xOffsets[4], const int_t yOffsets[4], int_t baseColor);
	// Ultra-optimized: builds ALL sign text into ONE Tesselator batch with vertex colors (1 draw call total)
	void drawSignTextSingleBatch(const jstring lines[4], const int_t xOffsets[4], const int_t yOffsets[4], int_t baseColor);

	int_t width(const jstring &str);
	jstring trimStringToWidth(const jstring &str, int_t width, bool reverse = false);

	static jstring sanitize(const jstring &str);
};
