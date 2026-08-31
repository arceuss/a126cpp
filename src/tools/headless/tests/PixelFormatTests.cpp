#include <cstring>
#include <string>

#include "legacygl/LegacyGL.h"
#include "legacygl/PixelFormat.h"
#include "tools/headless/TestFramework.h"

struct PixelDecodeCase
{
	const char *name;
	unsigned int format;
	legacygl::PixelTransferLayout layout;
	int components;
	unsigned char source[4];
	unsigned char expected[4];
};

struct PixelEncodeCase
{
	const char *name;
	unsigned int format;
	int components;
	unsigned char expected[4];
};

static bool sameBytes(const unsigned char *left, const unsigned char *right, int count)
{
	return std::memcmp(left, right, static_cast<std::size_t>(count)) == 0;
}

HEADLESS_TEST(pixel_format, seven_unsigned_byte_layouts)
{
	const PixelDecodeCase decodeCases[] = {
		{ "alpha", GL_ALPHA, legacygl::PixelTransferLayout::Alpha, 1,
			{ 11, 0, 0, 0 }, { 0, 0, 0, 11 } },
		{ "luminance", GL_LUMINANCE, legacygl::PixelTransferLayout::Luminance, 1,
			{ 11, 0, 0, 0 }, { 11, 11, 11, 255 } },
		{ "luminance-alpha", GL_LUMINANCE_ALPHA,
			legacygl::PixelTransferLayout::LuminanceAlpha, 2,
			{ 11, 22, 0, 0 }, { 11, 11, 11, 22 } },
		{ "rgb", GL_RGB, legacygl::PixelTransferLayout::RGB, 3,
			{ 11, 22, 33, 0 }, { 11, 22, 33, 255 } },
		{ "bgr", GL_BGR_EXT, legacygl::PixelTransferLayout::BGR, 3,
			{ 11, 22, 33, 0 }, { 33, 22, 11, 255 } },
		{ "rgba", GL_RGBA, legacygl::PixelTransferLayout::RGBA, 4,
			{ 11, 22, 33, 44 }, { 11, 22, 33, 44 } },
		{ "bgra", GL_BGRA_EXT, legacygl::PixelTransferLayout::BGRA, 4,
			{ 11, 22, 33, 44 }, { 33, 22, 11, 44 } }
	};

	for (const PixelDecodeCase &test : decodeCases)
	{
		const legacygl::PixelTransferFormat *description =
			legacygl::unsignedBytePixelTransferFormat(test.format);
		ctx.check(description != nullptr, std::string(test.name) + " format is described");
		if (description == nullptr)
			continue;
		ctx.check(description->layout == test.layout,
			std::string(test.name) + " layout is exact");
		ctx.checkEqual(description->components, test.components,
			std::string(test.name) + " component count is exact");

		unsigned char decoded[4] = {};
		ctx.check(legacygl::decodeUnsignedBytePixel(test.source, test.format, decoded),
			std::string(test.name) + " decode succeeds");
		ctx.check(sameBytes(decoded, test.expected, 4),
			std::string(test.name) + " decode matches RGBA8");
	}

	const unsigned char rgba[4] = { 19, 71, 131, 37 };
	const PixelEncodeCase encodeCases[] = {
		{ "alpha", GL_ALPHA, 1, { 37, 0, 0, 0 } },
		{ "luminance", GL_LUMINANCE, 1, { 221, 0, 0, 0 } },
		{ "luminance-alpha", GL_LUMINANCE_ALPHA, 2, { 221, 37, 0, 0 } },
		{ "rgb", GL_RGB, 3, { 19, 71, 131, 0 } },
		{ "bgr", GL_BGR_EXT, 3, { 131, 71, 19, 0 } },
		{ "rgba", GL_RGBA, 4, { 19, 71, 131, 37 } },
		{ "bgra", GL_BGRA_EXT, 4, { 131, 71, 19, 37 } }
	};
	for (const PixelEncodeCase &test : encodeCases)
	{
		unsigned char encoded[4] = {};
		ctx.check(legacygl::encodeUnsignedBytePixel(rgba, test.format, encoded),
			std::string(test.name) + " encode succeeds");
		ctx.check(sameBytes(encoded, test.expected, test.components),
			std::string(test.name) + " encode matches transfer layout");
	}

	ctx.check(legacygl::unsignedBytePixelTransferFormat(0xffffffffu) == nullptr,
		"unsupported transfer format has no description");
	unsigned char pixel[4] = {};
	ctx.check(!legacygl::decodeUnsignedBytePixel(rgba, 0xffffffffu, pixel),
		"unsupported transfer format decode fails");
	ctx.check(!legacygl::encodeUnsignedBytePixel(rgba, 0xffffffffu, pixel),
		"unsupported transfer format encode fails");
}

HEADLESS_TEST(pixel_format, luminance_clamp_and_intended_storage)
{
	const unsigned char bright[4] = { 200, 100, 1, 77 };
	unsigned char encoded[2] = {};
	ctx.check(legacygl::encodeUnsignedBytePixel(bright, GL_LUMINANCE_ALPHA, encoded),
		"luminance-alpha encode succeeds");
	ctx.checkEqual(encoded[0], 255, "luminance sum clamps to 255");
	ctx.checkEqual(encoded[1], 77, "luminance-alpha preserves alpha");

	legacygl::PixelStorageFormat storage;
	ctx.check(legacygl::pixelStorageFormat(GL_RGB, storage),
		"RGB intended storage is supported");
	ctx.check(storage.intended == legacygl::IntendedPixelFormat::RGB,
		"RGB intended format remains explicit");
	ctx.check(storage.physical == legacygl::PhysicalPixelFormat::RGBA8,
		"RGB physical representation is explicitly RGBA8");
	unsigned char rgbPixel[4] = { 1, 2, 3, 4 };
	ctx.check(legacygl::applyIntendedPixelFormat(storage.intended, rgbPixel),
		"RGB intended mapping applies");
	ctx.checkEqual(rgbPixel[3], 255, "RGB intended storage discards source alpha");

	ctx.check(legacygl::pixelStorageFormat(GL_RGBA, storage),
		"RGBA intended storage is supported");
	ctx.check(storage.intended == legacygl::IntendedPixelFormat::RGBA,
		"RGBA intended format remains explicit");
	unsigned char rgbaPixel[4] = { 1, 2, 3, 4 };
	ctx.check(legacygl::applyIntendedPixelFormat(storage.intended, rgbaPixel),
		"RGBA intended mapping applies");
	ctx.checkEqual(rgbaPixel[3], 4, "RGBA intended storage preserves source alpha");

	ctx.check(!legacygl::pixelStorageFormat(GL_ALPHA, storage),
		"unsupported intended internal format fails");
}
