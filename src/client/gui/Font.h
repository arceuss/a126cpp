#pragma once

#include <array>
#include <string>
#include <vector>

#include "client/MemoryTracker.h"

#include "java/Type.h"
#include "java/String.h"
#include "java/BufferedImage.h"
#include "OpenGL.h"

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
	// VBO for glyph rendering (replaces display lists)
	GLuint glyphVBOs[256] = {0};  // One VBO per glyph (8 bytes per quad = 4 vertices * 2 floats per vertex)
	GLuint glyphVAO = 0;  // Single VAO for all glyphs (shared layout)
	bool glyphsInitialized = false;
	
	// Buffer for batching character rendering
	std::vector<float> quadBuffer;  // Reusable buffer for building quads (x, y, u, v)
	GLuint batchVBO = 0;  // Reusable VBO for batched quads
	int_t currentColorR = 255, currentColorG = 255, currentColorB = 255;
	float currentAlpha = 1.0f;
	
	// Cached font atlas image for CPU-side text baking (loaded once, reused many times)
	mutable BufferedImage cachedFontAtlas;
	mutable bool fontAtlasCached = false;

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
	// Returns a const reference to avoid copying (BufferedImage is non-copyable)
	const BufferedImage &getFontAtlasImage() const;
	
private:
	// Initialize glyph VBOs and VAO (called once in constructor)
	void initializeGlyphs();
	
	// Render a batched quad buffer (helper for draw functions)
	void renderBatchedQuads();
};
