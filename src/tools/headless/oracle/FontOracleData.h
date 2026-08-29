#pragma once

#include <array>

#include "java/String.h"
#include "java/Type.h"

// Metrics produced by the Alpha reference font code running on real Java.
//
// Generated with `_font_oracle/FontOracle.java` (JDK 8u504) from
// `resource/font/default.png` and `resource/font.txt`, which are byte-identical
// to the reference client's own files. The scan and the string measurement are
// transliterations of `FontRenderer.init` and `FontRenderer.getStringWidth`
// (refs/apclient_cfr/net/minecraft/src/FontRenderer.java:62-89).
namespace FontOracle
{

// Per-glyph advance for codes 0-255.
static const std::array<int_t, 256> charWidths = {
	1, 9, 9, 8, 8, 8, 8, 7, 9, 8, 9, 9, 8, 9, 9, 9,
	8, 8, 8, 8, 9, 9, 8, 9, 8, 8, 8, 8, 8, 9, 9, 9,
	4, 2, 5, 6, 6, 6, 6, 3, 5, 5, 5, 6, 2, 6, 2, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 5, 6, 5, 6,
	7, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 4, 6, 6,
	3, 6, 6, 6, 6, 6, 5, 6, 6, 2, 6, 5, 3, 6, 6, 6,
	6, 6, 6, 6, 4, 6, 6, 6, 6, 6, 6, 5, 2, 5, 7, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 3, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6,
	6, 3, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6, 2, 6, 6,
	8, 9, 9, 6, 6, 6, 8, 8, 6, 8, 8, 8, 8, 8, 6, 6,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 6, 9, 9, 9, 5, 9, 9,
	8, 7, 7, 8, 7, 8, 8, 8, 7, 8, 8, 7, 9, 9, 6, 7,
	7, 7, 7, 7, 9, 6, 7, 8, 7, 6, 6, 9, 7, 6, 7, 1,
};

struct MeasuredLine
{
	const char16_t *text;
	int_t width;
};

// Sign lines covering plain text, narrow and wide glyphs, every colour-code
// shape the client can receive, and the complete glyph table.
static const std::array<MeasuredLine, 21> lines = { {
	{ u"", 0 },
	{ u" ", 4 },
	{ u"AlphaPlace", 54 },
	{ u"was here 2025", 74 },
	{ u"iiiiiiiiiiiiiii", 30 },
	{ u"WWWWWWWWWWWWWWW", 90 },
	{ u"\u00a70black \u00a7fwhite", 54 },
	{ u"\u00a7a\u00a7b\u00a7cstacked", 39 },
	{ u"trailing \u00a7", 38 },
	{ u"\u00a7zbad code", 46 },
	{ u"mixed \u00a79CASE\u00a7E x", 64 },
	{ u" !\"#$%&'()*+,-.", 69 },
	{ u"/0123456789:;<=", 81 },
	{ u">?@ABCDEFGHIJKL", 88 },
	{ u"MNOPQRSTUVWXYZ[", 88 },
	{ u"\\]^_'abcdefghij", 80 },
	{ u"klmnopqrstuvwxy", 84 },
	{ u"z{|}~\u2302\u00c7\u00fc\u00e9\u00e2\u00e4\u00e0\u00e5\u00e7\u00ea", 85 },
	{ u"\u00eb\u00e8\u00ef\u00ee\u00ec\u00c4\u00c5\u00c9\u00e6\u00c6\u00f4\u00f6\u00f2\u00fb\u00f9", 85 },
	{ u"\u00ff\u00d6\u00dc\u00f8\u00a3\u00d8\u00d7\u0192\u00e1\u00ed\u00f3\u00fa\u00f1\u00d1\u00aa", 85 },
	{ u"\u00ba\u00bf\u00ae\u00ac\u00bd\u00bc\u00a1\u00ab\u00bb", 51 },
} };

}
