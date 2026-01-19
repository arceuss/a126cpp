#pragma once

#include <array>
#include <string>
#include <vector>

#include "client/MemoryTracker.h"

#include "java/Type.h"
#include "java/String.h"
#include "java/BufferedImage.h"

class Options;
class Textures;

class Font
{
private:
	std::array<int_t, 256> charWidths;
	// Performance: Store color code RGB values (like original Java's field_22009_h array)
	// This avoids glGetFloatv queries when processing color codes
	std::array<int_t, 32> colorCodeRGBs;  // RGB packed as 0xRRGGBB
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

	static jstring sanitize(const jstring &str);
	
	// Access font atlas data for texture baking
	// Returns the font atlas image data (128x128 RGBA)
	// This is used by SignRenderer to bake text into textures
	const std::array<int_t, 256> &getCharWidths() const { return charWidths; }
	const std::array<int_t, 32> &getColorCodeRGBs() const { return colorCodeRGBs; }
	
	// Get font atlas image (reloads if needed)
	// Used for CPU-side text baking
	BufferedImage getFontAtlasImage() const;
};
