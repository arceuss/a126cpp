#include "client/gui/Font.h"

#include "SharedConstants.h"
#include "client/Options.h"
#include "client/renderer/Textures.h"
#include "client/renderer/Tesselator.h"

#include "java/Resource.h"
#include "java/BufferedImage.h"
#include "OpenGL.h"

// Alpha: FontRenderer.init - the last column with a used pixel defines the
// advance; the space glyph is fixed at two columns (FontRenderer.java:62-89).
std::array<int_t, 256> Font::computeCharWidths(const BufferedImage &image)
{
	std::array<int_t, 256> widths;

	int_t w = image.getWidth();
	const unsigned char *rawPixels = image.getRawPixels();

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
				int_t alpha = rawPixels[(xPixel + yPixel) * 4 + 3] & 0xFF;
				// Alpha treats a pixel as used above an alpha of 16.
				if (alpha > 16)
					emptyColumn = false;
			}
			if (!emptyColumn)
				break;
		}

		if (i == 32) x = 2;
		widths[i] = x + 2;
	}

	return widths;
}

// Alpha: FontRenderer.getStringWidth. A complete colour code consumes its next
// character and adds no width; a dangling section sign contributes -1 through
// getCharWidthFloat (FontRenderer.java:252-266,273-293).
int_t Font::widthOf(const std::array<int_t, 256> &charWidths, const jstring &str)
{
	int_t len = 0;

	for (int_t i = 0; i < static_cast<int_t>(str.length()); i++)
	{
		char_t c = str[i];
		if (c == 167)
		{
			if (i + 1 < static_cast<int_t>(str.length()))
				i++;
			else
				len--;
		}
		else
		{
			int_t ch = SharedConstants::letterIndex(c);
			if (ch >= 0)
				len += charWidths.at(ch + 32);
		}
	}

	return len;
}

Font::Font(Options &options, const jstring &name, Textures &textures)
{
	std::unique_ptr<std::istream> is(Resource::getResource(name));
	BufferedImage img = BufferedImage::ImageIO_read(*is);

	charWidths = computeCharWidths(img);

	fontTexture = textures.getTexture(img);

	listPos = MemoryTracker::genLists(256);
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

		// Alpha stores the code colors and applies them with glColor4f
		// (FontRenderer.java:112, 206-208), so the port keeps the packed value
		// instead of querying the current GL color back on every code.
		colorCodeRGB[j] = (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF);
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
			
			// Alpha: int var7 = this.field_22009_h[var5];
			//        GL11.glColor4f(var7 >> 16, var7 >> 8 & 255, var7 & 255, this.alpha)
			//        (FontRenderer.java:204-208). The shadow pass uses the
			//        darkened half of the table.
			int_t rgb = colorCodeRGB[codeIndex + (darken ? 16 : 0)];
			glColor4f(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, alpha);
			
			i++;  // Skip the color code character
		}
		else
		{
			int_t chIndex = SharedConstants::letterIndex(ch);
			if (chIndex >= 0)
				ib.push_back(listPos + chIndex + 32);
		}
	}

	// Render any remaining accumulated characters
	if (!ib.empty())
		glCallLists(ib.size(), GL_UNSIGNED_INT, ib.data());
	glPopMatrix();
}

void Font::drawLinesBatched(const jstring *lines, const int_t *xs, const int_t *ys, int_t lineCount, int_t color)
{
	// Colour handling is the same sequence `draw` uses.
	if ((color & 0xFF000000) == 0 && color != 0)
		color |= 0xFF000000;

	float alpha = ((color >> 24) & 0xFF) / 255.0f;
	if (alpha == 0.0f)
		alpha = 1.0f;

	float baseR = ((color >> 16) & 0xFF) / 255.0f;
	float baseG = ((color >> 8) & 0xFF) / 255.0f;
	float baseB = (color & 0xFF) / 255.0f;

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	Tesselator &t = Tesselator::instance;
	t.begin();

	static const jstring colorCodes = u"0123456789abcdef";
	const float s = 7.99f;

	for (int_t line = 0; line < lineCount; line++)
	{
		const jstring &str = lines[line];
		if (str.empty())
			continue;

		float r = baseR;
		float g = baseG;
		float b = baseB;
		float x = static_cast<float>(xs[line]);
		float y = static_cast<float>(ys[line]);

		for (int_t i = 0; i < static_cast<int_t>(str.length()); i++)
		{
			char_t ch = str[i];
			if (ch == 167 && i + 1 < static_cast<int_t>(str.length()))
			{
				char_t codeChar = str[i + 1];
				char_t lowerCode = codeChar;
				if (codeChar >= u'A' && codeChar <= u'F')
					lowerCode = codeChar + (u'a' - u'A');

				size_t codeIndex = colorCodes.find(static_cast<char16_t>(lowerCode));
				if (codeIndex == jstring::npos || codeIndex > 15)
					codeIndex = 15;

				// Same table the colour-code display lists are built from.
				int_t rgb = colorCodeRGB[codeIndex];
				r = ((rgb >> 16) & 0xFF) / 255.0f;
				g = ((rgb >> 8) & 0xFF) / 255.0f;
				b = (rgb & 0xFF) / 255.0f;

				i++;
				continue;
			}

			int_t chIndex = SharedConstants::letterIndex(ch);
			if (chIndex < 0)
				continue;

			int_t code = chIndex + 32;
			float ix = static_cast<float>(code % 16 * 8);
			float iy = static_cast<float>(code / 16 * 8);

			// Identical quad to the compiled glyph list above.
			t.color(r, g, b, alpha);
			t.vertexUV(x, y + s, 0.0, ix / 128.0f, (iy + s) / 128.0f);
			t.vertexUV(x + s, y + s, 0.0, (ix + s) / 128.0f, (iy + s) / 128.0f);
			t.vertexUV(x + s, y, 0.0, (ix + s) / 128.0f, iy / 128.0f);
			t.vertexUV(x, y, 0.0, ix / 128.0f, iy / 128.0f);

			x += static_cast<float>(charWidths[code]);
		}
	}

	t.end();
}

void Font::drawLinesImmediate(const jstring *lines, const int_t *xs, const int_t *ys, int_t lineCount, int_t color)
{
	// Keep this in lock-step with drawLinesBatched.  The only difference is
	// submission: these commands are intended to be captured by a static sign
	// display list instead of rebuilt into a GL_STREAM_DRAW buffer every frame.
	if ((color & 0xFF000000) == 0 && color != 0)
		color |= 0xFF000000;

	float alpha = ((color >> 24) & 0xFF) / 255.0f;
	if (alpha == 0.0f)
		alpha = 1.0f;

	float baseR = ((color >> 16) & 0xFF) / 255.0f;
	float baseG = ((color >> 8) & 0xFF) / 255.0f;
	float baseB = (color & 0xFF) / 255.0f;

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	static const jstring colorCodes = u"0123456789abcdef";
	const float s = 7.99f;

	glBegin(GL_TRIANGLES);
	for (int_t line = 0; line < lineCount; line++)
	{
		const jstring &str = lines[line];
		if (str.empty())
			continue;

		float r = baseR;
		float g = baseG;
		float b = baseB;
		float x = static_cast<float>(xs[line]);
		float y = static_cast<float>(ys[line]);

		for (int_t i = 0; i < static_cast<int_t>(str.length()); i++)
		{
			char_t ch = str[i];
			if (ch == 167 && i + 1 < static_cast<int_t>(str.length()))
			{
				char_t codeChar = str[i + 1];
				char_t lowerCode = codeChar;
				if (codeChar >= u'A' && codeChar <= u'F')
					lowerCode = codeChar + (u'a' - u'A');

				size_t codeIndex = colorCodes.find(static_cast<char16_t>(lowerCode));
				if (codeIndex == jstring::npos || codeIndex > 15)
					codeIndex = 15;

				int_t rgb = colorCodeRGB[codeIndex];
				r = ((rgb >> 16) & 0xFF) / 255.0f;
				g = ((rgb >> 8) & 0xFF) / 255.0f;
				b = (rgb & 0xFF) / 255.0f;
				i++;
				continue;
			}

			int_t chIndex = SharedConstants::letterIndex(ch);
			if (chIndex < 0)
				continue;

			int_t code = chIndex + 32;
			float ix = static_cast<float>(code % 16 * 8);
			float iy = static_cast<float>(code / 16 * 8);
			float u0 = ix / 128.0f;
			float v0 = iy / 128.0f;
			float u1 = (ix + s) / 128.0f;
			float v1 = (iy + s) / 128.0f;

			glColor4f(r, g, b, alpha);
			// Same A,B,C,A,C,D triangle order produced by Tesselator when its
			// GL_QUADS compatibility path expands one glyph quad.
			glTexCoord2f(u0, v1); glVertex3f(x, y + s, 0.0f);
			glTexCoord2f(u1, v1); glVertex3f(x + s, y + s, 0.0f);
			glTexCoord2f(u1, v0); glVertex3f(x + s, y, 0.0f);
			glTexCoord2f(u0, v1); glVertex3f(x, y + s, 0.0f);
			glTexCoord2f(u1, v0); glVertex3f(x + s, y, 0.0f);
			glTexCoord2f(u0, v0); glVertex3f(x, y, 0.0f);

			x += static_cast<float>(charWidths[code]);
		}
	}
	glEnd();
}

int_t Font::width(const jstring &str)
{
	return widthOf(charWidths, str);
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
				int_t ch = SharedConstants::letterIndex(c);
				if (ch >= 0)
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
				int_t ch = SharedConstants::letterIndex(c);
				if (ch >= 0)
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
		else if (SharedConstants::letterIndex(c) >= 0)
			result.push_back(c);
	}

	return result;
}

