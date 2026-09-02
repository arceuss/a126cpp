#include "client/gui/Font.h"

#include "SharedConstants.h"
#include "client/Options.h"
#include "client/renderer/Textures.h"
#include "client/renderer/Tesselator.h"
#include "client/model/ModelMatrix.h"

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
	// One draw per string instead of one display-list call per glyph. Alpha's
	// glyph lists are [quad, glTranslatef(width)], so a 40-character line was
	// 40 resolved draws with 40 modelview matrices on the translated backends;
	// on the Switch that put the F3 overlay at 9 ms a frame. The quad, UVs,
	// advances and colour sequence are the ones the lists carried, and every
	// 8-bit colour survives the float round trip through Tesselator::color
	// exactly, so the pixels are unchanged.
	drawLinesBatched(&str, &x, &y, 1, color, darken);
}

void Font::drawLinesBatched(const jstring *lines, const int_t *xs, const int_t *ys, int_t lineCount,
	int_t color, bool darken)
{
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	Tesselator &t = Tesselator::instance;
	t.begin();
	appendLines(t, lines, xs, ys, lineCount, color, darken, nullptr);
	t.end();
}

void Font::bindFontTexture()
{
	glBindTexture(GL_TEXTURE_2D, fontTexture);
}

void Font::appendLines(Tesselator &t, const jstring *lines, const int_t *xs, const int_t *ys,
	int_t lineCount, int_t color, bool darken, const ModelMatrix *transform)
{
	// Alpha: FontRenderer.renderString, in this order: default the alpha bits
	// if absent, darken the RGB for the shadow pass while preserving alpha,
	// then take the alpha (FontRenderer.java:225-231). newb12 additionally
	// treats an extracted alpha of 0 as opaque.
	if ((color & 0xFF000000) == 0 && color != 0)
		color |= 0xFF000000;
	if (darken)
		color = (color & 0xFCFCFC) >> 2 | (color & 0xFF000000);

	float alpha = ((color >> 24) & 0xFF) / 255.0f;
	if (alpha == 0.0f)
		alpha = 1.0f;

	float baseR = ((color >> 16) & 0xFF) / 255.0f;
	float baseG = ((color >> 8) & 0xFF) / 255.0f;
	float baseB = (color & 0xFF) / 255.0f;

	static const jstring colorCodes = u"0123456789abcdef";
	const float s = 7.99f;

	// A glyph corner, through the caller's matrix when one is given.
	auto emit = [&](float x, float y, float u, float v)
	{
		if (transform == nullptr)
		{
			t.vertexUV(x, y, 0.0, u, v);
			return;
		}
		float tx = 0.0f, ty = 0.0f, tz = 0.0f;
		transform->transformPoint(x, y, 0.0f, tx, ty, tz);
		t.vertexUV(tx, ty, tz, u, v);
	};

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

				// Same table as before; the shadow pass reads its darkened half
				// (FontRenderer.java:204-208).
				int_t rgb = colorCodeRGB[codeIndex + (darken ? 16 : 0)];
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

			// The quad Alpha's compiled glyph list carried.
			t.color(r, g, b, alpha);
			emit(x, y + s, ix / 128.0f, (iy + s) / 128.0f);
			emit(x + s, y + s, (ix + s) / 128.0f, (iy + s) / 128.0f);
			emit(x + s, y, (ix + s) / 128.0f, iy / 128.0f);
			emit(x, y, ix / 128.0f, iy / 128.0f);

			x += static_cast<float>(charWidths[code]);
		}
	}
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

