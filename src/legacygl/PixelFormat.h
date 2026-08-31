#pragma once

namespace legacygl
{

enum class PixelTransferLayout
{
	Alpha,
	Luminance,
	LuminanceAlpha,
	RGB,
	BGR,
	RGBA,
	BGRA
};

enum class IntendedPixelFormat
{
	RGB,
	RGBA
};

enum class PhysicalPixelFormat
{
	RGBA8
};

struct PixelTransferFormat
{
	PixelTransferLayout layout;
	unsigned int glFormat;
	int components;
};

struct PixelStorageFormat
{
	IntendedPixelFormat intended;
	PhysicalPixelFormat physical;
};

const PixelTransferFormat *unsignedBytePixelTransferFormat(unsigned int format);
bool pixelStorageFormat(int internalFormat, PixelStorageFormat &result);
bool decodeUnsignedBytePixel(const unsigned char *source, unsigned int format,
	unsigned char rgba[4]);
bool encodeUnsignedBytePixel(const unsigned char rgba[4], unsigned int format,
	unsigned char *destination);
bool applyIntendedPixelFormat(IntendedPixelFormat intended, unsigned char rgba[4]);

}
