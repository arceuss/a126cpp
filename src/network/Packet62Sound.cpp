#include "network/Packet62Sound.h"
#include "network/NetHandler.h"
#include <vector>
#include <stdexcept>

Packet62Sound::Packet62Sound()
	: sound(u"")
	, locX(0.0)
	, locY(0.0)
	, locZ(0.0)
	, f(0.0f)
	, f1(0.0f)
{
}

void Packet62Sound::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.sound = var1.readUTF();
	// Note: readUTF uses Modified UTF-8 format (short length prefix, then UTF-8 bytes)
	// We need to read this manually since SocketInputStream doesn't have readUTF yet
	short_t utfByteLength = in.readShort();
	if (utfByteLength < 0)
	{
		throw std::runtime_error("Invalid UTF string length: " + std::to_string(utfByteLength));
	}
	
	// Read UTF-8 bytes
	std::vector<byte_t> utfBytes(utfByteLength);
	for (int i = 0; i < utfByteLength; ++i)
	{
		utfBytes[i] = in.readByte();
	}
	
	// Convert Modified UTF-8 to UTF-16 (jstring)
	// Simple conversion for ASCII strings (most sounds are ASCII)
	sound = u"";
	for (int i = 0; i < utfByteLength; )
	{
		byte_t b = utfBytes[i++];
		if ((b & 0x80) == 0)
		{
			// ASCII character (0xxxxxxx)
			sound += static_cast<char16_t>(b);
		}
		else if ((b & 0xE0) == 0xC0 && i < utfByteLength)
		{
			// 2-byte UTF-8 (110xxxxx 10xxxxxx)
			byte_t b2 = utfBytes[i++];
			int codePoint = ((b & 0x1F) << 6) | (b2 & 0x3F);
			sound += static_cast<char16_t>(codePoint);
		}
		else if ((b & 0xF0) == 0xE0 && i + 1 < utfByteLength)
		{
			// 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
			byte_t b2 = utfBytes[i++];
			byte_t b3 = utfBytes[i++];
			int codePoint = ((b & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
			if (codePoint <= 0xFFFF)
			{
				sound += static_cast<char16_t>(codePoint);
			}
			else
			{
				// Surrogate pair (code point > 0xFFFF)
				codePoint -= 0x10000;
				sound += static_cast<char16_t>(0xD800 + (codePoint >> 10));
				sound += static_cast<char16_t>(0xDC00 + (codePoint & 0x3FF));
			}
		}
		else
		{
			// Invalid UTF-8, skip or use replacement character
			sound += static_cast<char16_t>(0xFFFD);
		}
	}
	
	// this.locX = var1.readDouble();
	this->locX = in.readDouble();
	
	// this.locY = var1.readDouble();
	this->locY = in.readDouble();
	
	// this.locZ = var1.readDouble();
	this->locZ = in.readDouble();
	
	// this.f = var1.readFloat();
	this->f = in.readFloat();
	
	// this.f1 = var1.readFloat();
	this->f1 = in.readFloat();
}

void Packet62Sound::writePacketData(SocketOutputStream& out)
{
	// Java: writePacketData is empty (server-to-client only)
	// Packet62Sound is only sent from server to client
}

void Packet62Sound::processPacket(NetHandler* handler)
{
	// Java: var1.handle62Sound(this);
	handler->handle62Sound(this);
}

int Packet62Sound::getPacketSize()
{
	// Java: return this.sound.length() + 24 + 8;
	// Sound string (UTF-8 bytes) + 3 doubles (24) + 2 floats (8)
	// Note: sound.length() in Java is character count, but we need byte count for UTF-8
	// For approximation, use string length * average UTF-8 bytes per char (usually 1 for ASCII)
	return static_cast<int>(sound.length()) + 24 + 8;
}

int Packet62Sound::getPacketId() const
{
	return 62;
}
