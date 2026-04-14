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
		
		// Store color code values for optimized rendering (normal and darkened variants)
		if (j < 16)
		{
			colorCodeR[j] = r / 255.0f;
			colorCodeG[j] = g / 255.0f;
			colorCodeB[j] = b / 255.0f;
		}
		else
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
	if (str.empty()) return;

	// Alpha 1.2.6: Match Java's exact order of operations for color processing
	if ((color & 0xFF000000) == 0 && color != 0)
	{
		color |= 0xFF000000;
	}

	if (darken)
	{
		color = (color & 0xFCFCFC) >> 2 | (color & 0xFF000000);
	}

	float alpha = ((color >> 24) & 0xFF) / 255.0f;
	if (alpha == 0.0f)
	{
		alpha = 1.0f;
	}

	int_t r = (color >> 16) & 0xFF;
	int_t g = (color >> 8) & 0xFF;
	int_t b = color & 0xFF;

	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, alpha);

	glPushMatrix();
	glTranslatef(x, y, 0.0f);

	static const jstring colorCodes = u"0123456789abcdef";

	Tesselator &t = Tesselator::instance;
	t.begin();
	t.color(r, g, b, (int_t)(alpha * 255.0f));

	float xPos = 0.0f;

	for (int_t i = 0; i < str.length(); i++)
	{
		char_t ch = str[i];
		if (ch == 167 && i + 1 < str.length())  // Color code
		{
			char_t codeChar = str[i + 1];
			char_t lowerCode = codeChar;
			if (codeChar >= u'A' && codeChar <= u'F')
				lowerCode = codeChar + (u'a' - u'A');

			int_t codeIndex = colorCodes.find(lowerCode);
			if (codeIndex == jstring::npos || codeIndex > 15)
				codeIndex = 15;

			int_t ccIdx = codeIndex + (darken ? 16 : 0);
			r = static_cast<int_t>(colorCodeR[ccIdx] * 255.0f);
			g = static_cast<int_t>(colorCodeG[ccIdx] * 255.0f);
			b = static_cast<int_t>(colorCodeB[ccIdx] * 255.0f);
			i++;
			continue;
		}

		int_t chIndex = SharedConstants::acceptableLetters.find(ch);
		if (chIndex == jstring::npos) continue;

		int_t glyphIndex = chIndex + 32;
		int_t ix = glyphIndex % 16 * 8;
		int_t iy = glyphIndex / 16 * 8;

		float s = 7.99f;
		float u0 = ix / 128.0f;
		float v0 = iy / 128.0f;
		float u1 = (ix + s) / 128.0f;
		float v1 = (iy + s) / 128.0f;

		t.color(r, g, b, (int_t)(alpha * 255.0f));

		t.vertexUV(xPos, 0.0f + s, 0.0, u0, v1);
		t.vertexUV(xPos + s, 0.0f + s, 0.0, u1, v1);
		t.vertexUV(xPos + s, 0.0, 0.0, u1, v0);
		t.vertexUV(xPos, 0.0, 0.0, u0, v0);

		xPos += charWidths[glyphIndex];
	}

	t.end();
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

// Ultra-optimized sign text: builds ALL glyphs into ONE Tesselator batch with vertex colors.
// Reduces sign rendering from ~60 draw calls to 1 draw call.
void Font::drawSignTextSingleBatch(const jstring lines[4], const int_t xOffsets[4], const int_t yOffsets[4], int_t baseColor)
{
	int_t color = baseColor;
	if ((color & 0xFF000000) == 0 && color != 0)
	{
		color |= 0xFF000000;
	}
	
	int_t r = (color >> 16) & 0xFF;
	int_t g = (color >> 8) & 0xFF;
	int_t b = color & 0xFF;
	int_t a = (color >> 24) & 0xFF;
	if (a == 0) a = 255;
	
	static const jstring colorCodes = u"0123456789abcdef";
	
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	
	Tesselator &t = Tesselator::instance;
	t.begin();
	t.color(r, g, b, a);
	
	for (int_t lineIdx = 0; lineIdx < 4; lineIdx++)
	{
		const jstring &str = lines[lineIdx];
		if (str.empty()) continue;
		
		float xPos = static_cast<float>(xOffsets[lineIdx]);
		float yPos = static_cast<float>(yOffsets[lineIdx]);
		
		// Reset to base color at start of each line
		r = (color >> 16) & 0xFF;
		g = (color >> 8) & 0xFF;
		b = color & 0xFF;
		
		for (int_t i = 0; i < str.length(); i++)
		{
			if (str[i] == 167 && i + 1 < str.length())  // Color code
			{
				char_t codeChar = str[i + 1];
				char_t lowerCode = codeChar;
				if (codeChar >= u'A' && codeChar <= u'F')
					lowerCode = codeChar + (u'a' - u'A');
				
				int_t codeIndex = colorCodes.find(lowerCode);
				if (codeIndex == jstring::npos || codeIndex > 15)
					codeIndex = 15;
				
				// Convert float color back to int
				r = static_cast<int_t>(colorCodeR[codeIndex] * 255.0f);
				g = static_cast<int_t>(colorCodeG[codeIndex] * 255.0f);
				b = static_cast<int_t>(colorCodeB[codeIndex] * 255.0f);
				i++;
				continue;
			}
			
			char_t ch = str[i];
			int_t chIndex = SharedConstants::acceptableLetters.find(ch);
			if (chIndex == jstring::npos) continue;
			
			int_t glyphIndex = chIndex + 32;
			int_t ix = glyphIndex % 16 * 8;
			int_t iy = glyphIndex / 16 * 8;
			
			float s = 7.99f;
			float u0 = ix / 128.0f;
			float v0 = iy / 128.0f;
			float u1 = (ix + s) / 128.0f;
			float v1 = (iy + s) / 128.0f;
			
			// Set color for this quad (vertex colors)
			t.color(r, g, b, a);
			
			// Emit quad vertices (same winding as original glyph display lists)
			t.vertexUV(xPos, yPos + s, 0.0, u0, v1);
			t.vertexUV(xPos + s, yPos + s, 0.0, u1, v1);
			t.vertexUV(xPos + s, yPos, 0.0, u1, v0);
			t.vertexUV(xPos, yPos, 0.0, u0, v0);
			
			xPos += charWidths[glyphIndex];
		}
	}
	
	t.end();
}

// Sign text uses the existing glyph display lists so it avoids rebuilding and uploading quads every frame.
void Font::drawSignTextBatched(const jstring lines[4], const int_t xOffsets[4], const int_t yOffsets[4], int_t baseColor)
{
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
	
	static const jstring colorCodes = u"0123456789abcdef";
	
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glColor4f(baseR, baseG, baseB, alpha);
	glPushMatrix();

	for (int_t lineIdx = 0; lineIdx < 4; lineIdx++)
	{
		const jstring &str = lines[lineIdx];
		if (str.empty())
		{
			continue;
		}

		glPushMatrix();
		glTranslatef(static_cast<float>(xOffsets[lineIdx]), static_cast<float>(yOffsets[lineIdx]), 0.0f);
		glColor4f(baseR, baseG, baseB, alpha);
		ib.clear();

		for (int_t i = 0; i < str.length(); i++)
		{
			if (str[i] == 167 && i + 1 < str.length())
			{
				if (!ib.empty())
				{
					glCallLists(ib.size(), GL_UNSIGNED_INT, ib.data());
					ib.clear();
				}

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
				
				glColor4f(colorCodeR[codeIndex], colorCodeG[codeIndex], colorCodeB[codeIndex], alpha);
				i++;
			}
			else
			{
				int_t chIndex = SharedConstants::acceptableLetters.find(str[i]);
				if (chIndex != jstring::npos)
				{
					ib.push_back(listPos + chIndex + 32);
				}
			}
		}

		if (!ib.empty())
		{
			glCallLists(ib.size(), GL_UNSIGNED_INT, ib.data());
		}

		glPopMatrix();
	}
	
	glPopMatrix();
}
