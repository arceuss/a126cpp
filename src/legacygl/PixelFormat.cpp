#include "legacygl/PixelFormat.h"

#include <algorithm>

#include "legacygl/LegacyGL.h"

namespace legacygl
{

static const PixelTransferFormat transferFormats[] = {
	{ PixelTransferLayout::Alpha, GL_ALPHA, 1 },
	{ PixelTransferLayout::Luminance, GL_LUMINANCE, 1 },
	{ PixelTransferLayout::LuminanceAlpha, GL_LUMINANCE_ALPHA, 2 },
	{ PixelTransferLayout::RGB, GL_RGB, 3 },
	{ PixelTransferLayout::BGR, GL_BGR_EXT, 3 },
	{ PixelTransferLayout::RGBA, GL_RGBA, 4 },
	{ PixelTransferLayout::BGRA, GL_BGRA_EXT, 4 }
};

const PixelTransferFormat *unsignedBytePixelTransferFormat(unsigned int format)
{
	for (const PixelTransferFormat &candidate : transferFormats)
	{
		if (candidate.glFormat == format)
			return &candidate;
	}
	return nullptr;
}

bool pixelStorageFormat(int internalFormat, PixelStorageFormat &result)
{
	switch (internalFormat)
	{
		case GL_RGB:
			result = { IntendedPixelFormat::RGB, PhysicalPixelFormat::RGBA8 };
			return true;
		case GL_RGBA:
			result = { IntendedPixelFormat::RGBA, PhysicalPixelFormat::RGBA8 };
			return true;
		default:
			return false;
	}
}

bool decodeUnsignedBytePixel(const unsigned char *source, unsigned int format,
	unsigned char rgba[4])
{
	const PixelTransferFormat *description = unsignedBytePixelTransferFormat(format);
	if (source == nullptr || rgba == nullptr || description == nullptr)
		return false;

	switch (description->layout)
	{
		case PixelTransferLayout::Alpha:
			rgba[0] = 0; rgba[1] = 0; rgba[2] = 0; rgba[3] = source[0];
			return true;
		case PixelTransferLayout::Luminance:
			rgba[0] = source[0]; rgba[1] = source[0]; rgba[2] = source[0]; rgba[3] = 255;
			return true;
		case PixelTransferLayout::LuminanceAlpha:
			rgba[0] = source[0]; rgba[1] = source[0]; rgba[2] = source[0]; rgba[3] = source[1];
			return true;
		case PixelTransferLayout::RGB:
			rgba[0] = source[0]; rgba[1] = source[1]; rgba[2] = source[2]; rgba[3] = 255;
			return true;
		case PixelTransferLayout::BGR:
			rgba[0] = source[2]; rgba[1] = source[1]; rgba[2] = source[0]; rgba[3] = 255;
			return true;
		case PixelTransferLayout::RGBA:
			rgba[0] = source[0]; rgba[1] = source[1]; rgba[2] = source[2]; rgba[3] = source[3];
			return true;
		case PixelTransferLayout::BGRA:
			rgba[0] = source[2]; rgba[1] = source[1]; rgba[2] = source[0]; rgba[3] = source[3];
			return true;
	}
	return false;
}

bool encodeUnsignedBytePixel(const unsigned char rgba[4], unsigned int format,
	unsigned char *destination)
{
	const PixelTransferFormat *description = unsignedBytePixelTransferFormat(format);
	if (rgba == nullptr || destination == nullptr || description == nullptr)
		return false;

	const unsigned int luminance = std::min(255u,
		static_cast<unsigned int>(rgba[0]) + static_cast<unsigned int>(rgba[1]) +
		static_cast<unsigned int>(rgba[2]));
	switch (description->layout)
	{
		case PixelTransferLayout::Alpha:
			destination[0] = rgba[3];
			return true;
		case PixelTransferLayout::Luminance:
			destination[0] = static_cast<unsigned char>(luminance);
			return true;
		case PixelTransferLayout::LuminanceAlpha:
			destination[0] = static_cast<unsigned char>(luminance);
			destination[1] = rgba[3];
			return true;
		case PixelTransferLayout::RGB:
			destination[0] = rgba[0]; destination[1] = rgba[1]; destination[2] = rgba[2];
			return true;
		case PixelTransferLayout::BGR:
			destination[0] = rgba[2]; destination[1] = rgba[1]; destination[2] = rgba[0];
			return true;
		case PixelTransferLayout::RGBA:
			destination[0] = rgba[0]; destination[1] = rgba[1];
			destination[2] = rgba[2]; destination[3] = rgba[3];
			return true;
		case PixelTransferLayout::BGRA:
			destination[0] = rgba[2]; destination[1] = rgba[1];
			destination[2] = rgba[0]; destination[3] = rgba[3];
			return true;
	}
	return false;
}

bool applyIntendedPixelFormat(IntendedPixelFormat intended, unsigned char rgba[4])
{
	if (rgba == nullptr)
		return false;
	switch (intended)
	{
		case IntendedPixelFormat::RGB:
			rgba[3] = 255;
			return true;
		case IntendedPixelFormat::RGBA:
			return true;
	}
	return false;
}

}
