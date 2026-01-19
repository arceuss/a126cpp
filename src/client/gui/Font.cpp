#include "client/gui/Font.h"

#include "SharedConstants.h"
#include "client/Options.h"
#include "client/renderer/Textures.h"
#include "client/renderer/Tesselator.h"
#include "OpenGL.h"

#include "java/Resource.h"
#include "java/BufferedImage.h"

Font::Font(Options &options, const jstring &name, Textures &textures)
{
	std::unique_ptr<std::istream> is(Resource::getResource(name));
	BufferedImage img = BufferedImage::ImageIO_read(*is);

	int_t w = img.getWidth();
	int_t h = img.getHeight();
	const unsigned char *rawPixels = img.getRawPixels();

	// Determine character widths
	for (int_t i = 0; i < 256; i++)
	{
		int_t xt = i % 16;
		int_t yt = i / 16;

		int_t x = 7;
		for (; x >= 0; x--)
		{
			int_t xPixel = xt * 8 + x;
			bool emptyColumn = true;
			for (int_t y = 0; y < 8 && emptyColumn; y++)
			{
				int_t yPixel = (yt * 8 + y) * w;
				int_t pixel = rawPixels[(xPixel + yPixel) * 4 + 3] & 0xFF;
				if (pixel > 0)
					emptyColumn = false;
			}
			if (!emptyColumn)
				break;
		}

		if (i == 32) x = 2;
		charWidths[i] = x + 2;
	}

	fontTexture = textures.getTexture(img);

	// Initialize glyph VBOs and VAO (replaces display lists)
	initializeGlyphs();

	for (int_t j = 0; j < 32; j++)
	{
		int_t br = ((j >> 3) & 1) * 85;
		int_t r = ((j >> 2) & 1) * 170 + br;
		int_t g = ((j >> 1) & 1) * 170 + br;
		int_t b = ((j >> 0) & 1) * 170 + br;
		if (j == 6)
			r += 85;

		bool darken = (j >= 16);

		if (options.anaglyph3d)
		{
			int_t cr = (r * 30 + g * 59 + b * 11) / 100;
			int_t cg = (r * 30 + g * 70) / 100;
			int_t cb = (r * 30 + b * 70) / 100;
			r = cr;
			g = cg;
			b = cb;
		}

		if (darken)
		{
			r /= 4;
			g /= 4;
			b /= 4;
		}

		// Alpha 1.2.6: Color code RGB values stored for fast lookup (replaces display lists)
		// Java: this.field_22009_h[var7] = (var9 & 255) << 16 | (var10 & 255) << 8 | var11 & 255;
		colorCodeRGBs[j] = (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF);
	}
}

void Font::drawShadow(const jstring &str, int_t x, int_t y, int_t color)
{
	draw(str, x + 1, y + 1, color, true);
	draw(str, x, y, color);
}

void Font::draw(const jstring &str, int_t x, int_t y, int_t color)
{
	draw(str, x, y, color, false);
}

void Font::draw(const jstring &str, int_t x, int_t y, int_t color, bool darken)
{
	// Alpha 1.2.6: Match Java's exact order of operations
	// Java: if((var4 & -16777216) == 0) { var4 |= -16777216; }
	//       if(var5) { var4 = (var4 & 16579836) >> 2 | var4 & -16777216; }
	//       this.alpha = (float)(var4 >> 24 & 255) / 255.0F;
	//       GL11.glColor4f(..., this.alpha);
	// Order: 1) Set default alpha if needed, 2) Darken if needed, 3) Extract alpha, 4) Set glColor4f
	
	// Step 1: Java sets default alpha if alpha bits are 0
	// -16777216 = 0xFF000000 (all alpha bits set)
	// IMPORTANT: This must happen BEFORE darken, as darken preserves alpha
	if ((color & 0xFF000000) == 0 && color != 0)
	{
		// This is a legacy color without alpha bits (like 0xFFFFFF)
		// OR chat color with var9=0 (0x00FFFFFF) - but these are skipped anyway
		color |= 0xFF000000;  // Set alpha to 255 (matching Java: var4 |= -16777216)
	}
	
	// Step 2: Java darkens RGB but preserves alpha
	// Java: if(var5) { var4 = (var4 & 16579836) >> 2 | var4 & -16777216; }
	// 16579836 = 0xFCFCFC (RGB mask), -16777216 = 0xFF000000 (alpha mask)
	if (darken)
	{
		color = (color & 0xFCFCFC) >> 2 | (color & 0xFF000000);
	}
	
	// Step 3: Extract alpha after all modifications
	// Java: this.alpha = (float)(var4 >> 24 & 255) / 255.0F;
	// newb12: float a = (color >> 24 & 0xFF) / 255.0F; if (a == 0.0F) { a = 1.0F; }
	float alpha = ((color >> 24) & 0xFF) / 255.0f;
	// newb12 also sets default alpha if extracted alpha is 0 (after darken operation)
	if (alpha == 0.0f)
	{
		alpha = 1.0f;  // Default to fully opaque
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	// Alpha 1.2.6: Set initial color (Java sets this.alpha and initial glColor4f)
	// Java: GL11.glColor4f((float)(var4 >> 16 & 255) / 255.0F, (float)(var4 >> 8 & 255) / 255.0F, (float)(var4 & 255) / 255.0F, this.alpha);
	int_t r = ((color >> 16) & 0xFF);
	int_t g = ((color >> 8) & 0xFF);
	int_t b = (color & 0xFF);
	currentColorR = r;
	currentColorG = g;
	currentColorB = b;
	currentAlpha = alpha;

	// Clear quad buffer for batching
	quadBuffer.clear();
	
	float currentX = static_cast<float>(x);
	
	// Alpha 1.2.6: Parse color codes (Java uses character 167 = 0xA7 = §)
	// Java: renderStringImpl() processes each character individually and renders immediately
	// Java: if(var4 == 167 && var3 + 1 < var1.length()) {
	//     var5 = "0123456789abcdef".indexOf(var1.toLowerCase().charAt(var3 + 1));
	//     ...
	//     int var7 = this.field_22009_h[var5];
	//     GL11.glColor4f((float)(var7 >> 16) / 255.0F, (float)(var7 >> 8 & 255) / 255.0F, (float)(var7 & 255) / 255.0F, this.alpha);
	//     ++var3;
	// }
	// To match Java behavior: batch characters in segments when color codes change
	static const jstring colorCodes = u"0123456789abcdef";
	
	for (int_t i = 0; i < str.length(); i++)
	{
		char_t ch = str[i];
		if (ch == 167 && i + 1 < str.length())  // 167 = 0xA7 = § (section symbol)
		{
				// Render accumulated quads before changing color
			if (!quadBuffer.empty())
			{
				// Set current color before rendering batch
				glColor4f(currentColorR / 255.0f, currentColorG / 255.0f, currentColorB / 255.0f, currentAlpha);
				renderBatchedQuads();
				quadBuffer.clear();
			}
			
			char_t codeChar = str[i + 1];
			// Convert to lowercase for comparison
			char_t lowerCode = codeChar;
			if (codeChar >= u'A' && codeChar <= u'F')
				lowerCode = codeChar + (u'a' - u'A');
			else if (codeChar >= u'a' && codeChar <= u'f')
				lowerCode = codeChar;
			else if (codeChar >= u'0' && codeChar <= u'9')
				lowerCode = codeChar;
			
			int_t codeIndex = colorCodes.find(lowerCode);
			if (codeIndex == jstring::npos || codeIndex > 15)
				codeIndex = 15;
			
			// Performance optimization: Set color directly like original Java code
			// Java: int var7 = this.field_22009_h[var5 + (var2 ? 16 : 0)];
			//       GL11.glColor4f((float)(var7 >> 16) / 255.0F, (float)(var7 >> 8 & 255) / 255.0F, (float)(var7 & 255) / 255.0F, this.alpha);
			// Look up stored RGB values (includes anaglyph3d transformation if enabled)
			int_t colorIndex = codeIndex + (darken ? 16 : 0);
			int_t rgb = colorCodeRGBs[colorIndex];
			currentColorR = (rgb >> 16) & 0xFF;
			currentColorG = (rgb >> 8) & 0xFF;
			currentColorB = rgb & 0xFF;
			
			i++;  // Skip the color code character
		}
		else
		{
			int_t chIndex = SharedConstants::acceptableLetters.find(ch);
			if (chIndex != jstring::npos)
			{
				int_t glyphIndex = chIndex + 32;
				// Add glyph quad to buffer (x, y, u, v per vertex)
				// Each glyph is 8x8 pixels, stored in 128x128 atlas
				int_t ix = glyphIndex % 16 * 8;
				int_t iy = glyphIndex / 16 * 8;
				float s = 7.99f;
				float u0 = ix / 128.0f;
				float v0 = iy / 128.0f;
				float u1 = (ix + s) / 128.0f;
				float v1 = (iy + s) / 128.0f;
				
				// Quad vertices: bottom-left, bottom-right, top-right, top-left
				// Vertex format: x, y, u, v (color set via glColor4f before rendering)
				quadBuffer.push_back(currentX);                    // x
				quadBuffer.push_back(static_cast<float>(y) + s);   // y
				quadBuffer.push_back(u0);                          // u
				quadBuffer.push_back(v1);                          // v
				
				quadBuffer.push_back(currentX + s);                // x
				quadBuffer.push_back(static_cast<float>(y) + s);   // y
				quadBuffer.push_back(u1);                          // u
				quadBuffer.push_back(v1);                          // v
				
				quadBuffer.push_back(currentX + s);                // x
				quadBuffer.push_back(static_cast<float>(y));       // y
				quadBuffer.push_back(u1);                          // u
				quadBuffer.push_back(v0);                          // v
				
				quadBuffer.push_back(currentX);                    // x
				quadBuffer.push_back(static_cast<float>(y));       // y
				quadBuffer.push_back(u0);                          // u
				quadBuffer.push_back(v0);                          // v
				
				currentX += charWidths[glyphIndex];
			}
		}
	}

	// Render any remaining accumulated quads
	if (!quadBuffer.empty())
	{
		// Set current color before rendering final batch
		glColor4f(currentColorR / 255.0f, currentColorG / 255.0f, currentColorB / 255.0f, currentAlpha);
		renderBatchedQuads();
	}
}

int_t Font::width(const jstring &str)
{
	int_t len = 0;

	for (int_t i = 0; i < str.length(); i++)
	{
		char_t c = str[i];
		// Alpha 1.2.6: Skip color codes (167 = 0xA7 = §)
		// Java: if(var4 == 167 && var3 + 1 < var1.length()) { ++var3; }
		if (c == 167 && i + 1 < str.length())
		{
			i++;  // Skip the color code character
		}
		else
		{
			int_t ch = SharedConstants::acceptableLetters.find(c);
			if (ch != jstring::npos)
				len += charWidths.at(ch + 32);
		}
	}

	return len;
}

jstring Font::trimStringToWidth(const jstring &str, int_t width, bool reverse)
{
	if (reverse)
	{
		jstring result;
		int_t len = 0;
		for (int_t i = str.length() - 1; i >= 0; i--)
		{
			char_t c = str[i];
			// Alpha 1.2.6: Skip color codes (167 = 0xA7 = §)
			if (c == 167 && i > 0)
			{
				i--;  // Skip the color code character
				continue;
			}
			
			int_t charWidth = 0;
			if (c == 223 && i > 0)
			{
				i--;
				charWidth = charWidths[223];
			}
			else
			{
				int_t ch = SharedConstants::acceptableLetters.find(c);
				if (ch != jstring::npos)
					charWidth = charWidths.at(ch + 32);
			}
			if (len + charWidth > width)
				break;
			len += charWidth;
			result.insert(0, 1, c);
		}
		return result;
	}
	else
	{
		jstring result;
		int_t len = 0;
		for (int_t i = 0; i < str.length(); i++)
		{
			char_t c = str[i];
			// Alpha 1.2.6: Skip color codes (167 = 0xA7 = §)
			if (c == 167 && i + 1 < str.length())
			{
				i++;  // Skip the color code character
				continue;
			}
			
			int_t charWidth = 0;
			if (c == 223 && i + 1 < str.length())
			{
				i++;
				charWidth = charWidths[223];
			}
			else
			{
				int_t ch = SharedConstants::acceptableLetters.find(c);
				if (ch != jstring::npos)
					charWidth = charWidths.at(ch + 32);
			}
			if (len + charWidth > width)
				break;
			len += charWidth;
			result.push_back(c);
		}
		return result;
	}
}

	jstring Font::sanitize(const jstring &str)
{
	jstring result;

	for (int_t i = 0; i < str.length(); i++)
	{
		char_t c = str[i];
		if (c == 223)
			i++;
		else if (SharedConstants::acceptableLetters.find(c) != jstring::npos)
			result.push_back(c);
	}

	return result;
}

// Get font atlas image for CPU-side text baking
// Caches the font texture image so we can sample glyphs without reloading from disk
// Returns a const reference to avoid copying (BufferedImage contains unique_ptr and is non-copyable)
const BufferedImage &Font::getFontAtlasImage() const
{
	// Cache the font atlas image to avoid expensive disk reads every frame
	// This is used for CPU-side text baking in SignRenderer
	// The image is only loaded once and then reused
	if (!fontAtlasCached)
	{
		using namespace Resource;
		std::unique_ptr<std::istream> is(getResource(u"/font/default.png"));
		cachedFontAtlas = BufferedImage::ImageIO_read(*is);
		fontAtlasCached = true;
	}
	return cachedFontAtlas;  // Return reference to avoid copy
}

// Initialize glyph VBOs and VAO (replaces display lists)
void Font::initializeGlyphs()
{
	if (glyphsInitialized)
		return;
	
	// Generate glyph VBOs (one per glyph, storing quad vertices)
	glGenBuffers(256, glyphVBOs);
	
	// Create VAO for font rendering (shared layout for all glyphs)
	glGenVertexArrays(1, &glyphVAO);
	glBindVertexArray(glyphVAO);
	
	// Each glyph VBO contains 4 vertices with format: x, y, u, v
	// Position: location 0, 2 floats
	// Texture: location 1, 2 floats
	for (int_t j = 0; j < 256; j++)
	{
		int_t ix = j % 16 * 8;
		int_t iy = j / 16 * 8;
		float s = 7.99f;
		
		float u0 = ix / 128.0f;
		float v0 = iy / 128.0f;
		float u1 = (ix + s) / 128.0f;
		float v1 = (iy + s) / 128.0f;
		
		// Quad vertices: x, y, u, v per vertex (4 vertices)
		float quadData[] = {
			0.0f, s,     u0, v1,  // bottom-left
			s,    s,     u1, v1,  // bottom-right
			s,    0.0f,  u1, v0,  // top-right
			0.0f, 0.0f,  u0, v0   // top-left
		};
		
		glBindBuffer(GL_ARRAY_BUFFER, glyphVBOs[j]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
	}
	
	// Set up fixed-function vertex attribute pointers in VAO (position and texture coords)
	// Use first VBO for attribute setup (VAO stores the layout)
	glBindBuffer(GL_ARRAY_BUFFER, glyphVBOs[0]);
	char *vbo_base = reinterpret_cast<char *>(0);
	
	// Position: 2 floats, stride 4 floats, offset 0
	glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), reinterpret_cast<GLvoid *>(vbo_base + 0));
	glEnableClientState(GL_VERTEX_ARRAY);
	
	// Texture: 2 floats, stride 4 floats, offset 2 floats
	glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), reinterpret_cast<GLvoid *>(vbo_base + 2 * sizeof(float)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// Unbind VAO (VBO binding and attribute layout remain stored in VAO)
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glyphsInitialized = true;
}

// Render batched quads from quadBuffer
void Font::renderBatchedQuads()
{
	if (quadBuffer.empty())
		return;
	
	// Create reusable batch VBO if needed
	if (batchVBO == 0)
	{
		glGenBuffers(1, &batchVBO);
	}
	
	// Use VAO for rendering if available, otherwise fall back to immediate mode
	if (glyphVAO != 0)
	{
		// Upload batched quad data to VBO
		glBindBuffer(GL_ARRAY_BUFFER, batchVBO);
		glBufferData(GL_ARRAY_BUFFER, quadBuffer.size() * sizeof(float), quadBuffer.data(), GL_STREAM_DRAW);
		
		// Bind VAO (which stores the attribute layout)
		glBindVertexArray(glyphVAO);
		
		// Update fixed-function vertex attribute pointers for this VBO
		glBindBuffer(GL_ARRAY_BUFFER, batchVBO);
		char *vbo_base = reinterpret_cast<char *>(0);
		
		// Position: 2 floats, stride 4 floats, offset 0
		glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), reinterpret_cast<GLvoid *>(vbo_base + 0));
		glEnableClientState(GL_VERTEX_ARRAY);
		
		// Texture: 2 floats, stride 4 floats, offset 2 floats
		glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), reinterpret_cast<GLvoid *>(vbo_base + 2 * sizeof(float)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		// Draw batched quads (GL_QUADS mode, 4 vertices per quad)
		int_t quadCount = static_cast<int_t>(quadBuffer.size()) / (4 * 4);  // 4 floats per vertex, 4 vertices per quad
		glDrawArrays(GL_QUADS, 0, quadCount * 4);
		
		// Disable client states and cleanup
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		// Fallback: immediate mode rendering (shouldn't happen if glyphsInitialized is true)
		glBegin(GL_QUADS);
		for (size_t i = 0; i < quadBuffer.size(); i += 4)
		{
			glTexCoord2f(quadBuffer[i + 2], quadBuffer[i + 3]);
			glVertex2f(quadBuffer[i], quadBuffer[i + 1]);
		}
		glEnd();
	}
}
