#include "client/gui/Font.h"

#include "SharedConstants.h"
#include "client/Options.h"
#include "client/renderer/Textures.h"
#include "client/renderer/Tesselator.h"

#include "java/Resource.h"
#include "java/BufferedImage.h"
#include "OpenGL.h"

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

	listPos = MemoryTracker::genLists(256 + 32);
	Tesselator &t = Tesselator::instance;
	for (int_t j = 0; j < 256; j++)
	{
		glNewList(listPos + j, GL_COMPILE);

		t.begin();
		
		int_t ix = j % 16 * 8;
		int_t iy = j / 16 * 8;

		float s = 7.99f;

		float uo = 0.0f;
		float vo = 0.0f;

		t.vertexUV(0.0, (0.0f + s), 0.0, (ix / 128.0f + uo), ((iy + s) / 128.0f + vo));
		t.vertexUV((0.0f + s), (0.0f + s), 0.0, ((ix + s) / 128.0f + uo), ((iy + s) / 128.0f + vo));
		t.vertexUV((0.0f + s), 0.0, 0.0, ((ix + s) / 128.0f + uo), (iy / 128.0f + vo));
		t.vertexUV(0.0, 0.0, 0.0, (ix / 128.0f + uo), (iy / 128.0f + vo));

		t.end();

		glTranslatef(charWidths[j], 0.0f, 0.0f);
		glEndList();
	}

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

		// Alpha 1.2.6: Color code display lists store RGB colors
		// Java: this.field_22009_h[var7] = (var9 & 255) << 16 | (var10 & 255) << 8 | var11 & 255;
		// The alpha is preserved from the original color parameter (this.alpha)
		glNewList(listPos + 256 + j, GL_COMPILE);
		glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);  // RGB only, alpha preserved from glColor4f call
		glEndList();
		
		// Store color code values for optimized sign rendering (only for non-darkened codes, j < 16)
		if (j < 16)
		{
			colorCodeR[j] = r / 255.0f;
			colorCodeG[j] = g / 255.0f;
			colorCodeB[j] = b / 255.0f;
		}
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
	float r = ((color >> 16) & 0xFF) / 255.0f;
	float g = ((color >> 8) & 0xFF) / 255.0f;
	float b = (color & 0xFF) / 255.0f;
	glColor4f(r, g, b, alpha);

	ib.clear();
	glPushMatrix();
	glTranslatef(x, y, 0.0f);

	// Alpha 1.2.6: Parse color codes (Java uses character 167 = 0xA7 = §)
	// Java: renderStringImpl() processes each character individually and renders immediately
	// Java: if(var4 == 167 && var3 + 1 < var1.length()) {
	//     var5 = "0123456789abcdef".indexOf(var1.toLowerCase().charAt(var3 + 1));
	//     ...
	//     int var7 = this.field_22009_h[var5];
	//     GL11.glColor4f((float)(var7 >> 16) / 255.0F, (float)(var7 >> 8 & 255) / 255.0F, (float)(var7 & 255) / 255.0F, this.alpha);
	//     ++var3;
	// }
	// To match Java behavior: render characters in segments when color codes change
	static const jstring colorCodes = u"0123456789abcdef";
	
	for (int_t i = 0; i < str.length(); i++)
	{
		char_t ch = str[i];
		if (ch == 167 && i + 1 < str.length())  // 167 = 0xA7 = § (section symbol)
		{
			// Render accumulated characters before changing color
			if (!ib.empty())
			{
				glCallLists(ib.size(), GL_UNSIGNED_INT, ib.data());
				ib.clear();
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
			
			// Set color using display list (color codes are in listPos + 256 + codeIndex)
			// For shadow, add 16 to get darker version
			// Java: if(var2) { var5 += 16; }
			// Java: int var7 = this.field_22009_h[var5];
			//       GL11.glColor4f((float)(var7 >> 16) / 255.0F, (float)(var7 >> 8 & 255) / 255.0F, (float)(var7 & 255) / 255.0F, this.alpha);
			// Java explicitly sets alpha when processing color codes, preserving this.alpha from the original color.
			// Our display list calls glColor3f (RGB only), which preserves the current alpha.
			// Since we already set glColor4f with alpha at the start, glColor3f will preserve it.
			// However, to ensure alpha is correct (especially for chat fading), we restore it explicitly.
			int_t colorListIndex = listPos + 256 + codeIndex + (darken ? 16 : 0);
			glCallList(colorListIndex);  // This calls glColor3f, setting RGB but preserving alpha
			// Restore alpha explicitly to ensure it's correct for fading
			// Get current RGB from OpenGL state, then restore alpha
			GLfloat currentColor[4];
			glGetFloatv(GL_CURRENT_COLOR, currentColor);
			glColor4f(currentColor[0], currentColor[1], currentColor[2], alpha);
			
			i++;  // Skip the color code character
		}
		else
		{
			int_t chIndex = SharedConstants::acceptableLetters.find(ch);
			if (chIndex != jstring::npos)
				ib.push_back(listPos + chIndex + 32);
		}
	}

	// Render any remaining accumulated characters
	if (!ib.empty())
		glCallLists(ib.size(), GL_UNSIGNED_INT, ib.data());
	glPopMatrix();
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

// Optimized batch rendering for signs - renders all text in a single draw call
// This significantly improves performance by batching all characters together
void Font::drawSignTextBatched(const jstring lines[4], int_t xOffsets[4], int_t yOffsets[4], int_t baseColor)
{
	// Parse base color (same logic as draw())
	int_t color = baseColor;
	if ((color & 0xFF000000) == 0 && color != 0)
	{
		color |= 0xFF000000;
	}
	float alpha = ((color >> 24) & 0xFF) / 255.0f;
	if (alpha == 0.0f)
	{
		alpha = 1.0f;
	}
	
	float baseR = ((color >> 16) & 0xFF) / 255.0f;
	float baseG = ((color >> 8) & 0xFF) / 255.0f;
	float baseB = (color & 0xFF) / 255.0f;
	
	// Use pre-calculated color code RGB values (includes anaglyph3d transformation if enabled)
	static const jstring colorCodes = u"0123456789abcdef";
	
	// Bind font texture
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	
	// Use Tesselator to batch all quads
	Tesselator &t = Tesselator::instance;
	t.begin();
	
	// Process each line
	for (int_t lineIdx = 0; lineIdx < 4; lineIdx++)
	{
		const jstring &str = lines[lineIdx];
		if (str.empty())
			continue;
		
		float currentR = baseR;
		float currentG = baseG;
		float currentB = baseB;
		float x = static_cast<float>(xOffsets[lineIdx]);
		float y = static_cast<float>(yOffsets[lineIdx]);
		
		// Parse string and render characters
		for (int_t i = 0; i < str.length(); i++)
		{
			char_t ch = str[i];
			if (ch == 167 && i + 1 < str.length())  // Color code (167 = 0xA7 = §)
			{
				char_t codeChar = str[i + 1];
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
				
				// Update current color using pre-calculated values
				currentR = colorCodeR[codeIndex];
				currentG = colorCodeG[codeIndex];
				currentB = colorCodeB[codeIndex];
				
				i++;  // Skip the color code character
			}
			else
			{
				// Find character index
				int_t chIndex = SharedConstants::acceptableLetters.find(ch);
				if (chIndex != jstring::npos)
				{
					// Calculate texture coordinates
					int_t charCode = chIndex + 32;
					int_t ix = charCode % 16 * 8;
					int_t iy = charCode / 16 * 8;
					
					float s = 7.99f;
					float u0 = ix / 128.0f;
					float u1 = (ix + s) / 128.0f;
					float v0 = iy / 128.0f;
					float v1 = (iy + s) / 128.0f;
					
					// Set color for this character
					t.color(currentR, currentG, currentB, alpha);
					
					// Render character quad
					t.vertexUV(x, y + s, 0.0, u0, v1);
					t.vertexUV(x + s, y + s, 0.0, u1, v1);
					t.vertexUV(x + s, y, 0.0, u1, v0);
					t.vertexUV(x, y, 0.0, u0, v0);
					
					// Advance x position
					x += static_cast<float>(charWidths[charCode]);
				}
			}
		}
	}
	
	// Render all batched quads in one draw call
	t.end();
}
