#include "network/Packet100OpenWindow.h"
#include "network/NetHandler.h"
#include <vector>
#include <stdexcept>

jstring Packet100OpenWindow::readUTF(SocketInputStream& in)
{
	// Java readUTF: reads short length (in bytes), then Modified UTF-8 bytes
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
	jstring result = u"";
	for (int i = 0; i < utfByteLength; )
	{
		byte_t b = utfBytes[i++];
		if ((b & 0x80) == 0)
		{
			// ASCII character (0xxxxxxx)
			result += static_cast<char16_t>(b);
		}
		else if ((b & 0xE0) == 0xC0 && i < utfByteLength)
		{
			// 2-byte UTF-8 (110xxxxx 10xxxxxx)
			byte_t b2 = utfBytes[i++];
			int codePoint = ((b & 0x1F) << 6) | (b2 & 0x3F);
			result += static_cast<char16_t>(codePoint);
		}
		else if ((b & 0xF0) == 0xE0 && i + 1 < utfByteLength)
		{
			// 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
			byte_t b2 = utfBytes[i++];
			byte_t b3 = utfBytes[i++];
			int codePoint = ((b & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
			if (codePoint <= 0xFFFF)
			{
				result += static_cast<char16_t>(codePoint);
			}
			else
			{
				// Surrogate pair (code point > 0xFFFF)
				codePoint -= 0x10000;
				result += static_cast<char16_t>(0xD800 + (codePoint >> 10));
				result += static_cast<char16_t>(0xDC00 + (codePoint & 0x3FF));
			}
		}
		else
		{
			// Invalid UTF-8, use replacement character
			result += static_cast<char16_t>(0xFFFD);
		}
	}
	
	return result;
}

Packet100OpenWindow::Packet100OpenWindow()
	: windowId(0)
	, inventoryType(0)
	, windowTitle(u"")
	, slotsCount(0)
{
}

void Packet100OpenWindow::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.windowId = var1.readByte();
	this->windowId = static_cast<int_t>(in.read() & 0xFF);
	
	// this.inventoryType = var1.readByte();
	this->inventoryType = static_cast<int_t>(in.read() & 0xFF);
	
	// this.windowTitle = var1.readUTF();
	this->windowTitle = readUTF(in);
	
	// this.slotsCount = var1.readByte();
	this->slotsCount = static_cast<int_t>(in.read() & 0xFF);
}

void Packet100OpenWindow::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
	
	// var1.writeByte(this.inventoryType);
	out.writeByte(static_cast<byte_t>(this->inventoryType));
	
	// var1.writeUTF(this.windowTitle);
	// Note: writeUTF writes short length (in bytes) then UTF-8 bytes
	// For now, this is server-to-client only, so writePacketData may be empty
	// But let's implement it to match Java
	// Convert UTF-16 to Modified UTF-8
	std::vector<byte_t> utf8Bytes;
	for (size_t i = 0; i < windowTitle.length(); ++i)
	{
		char16_t ch = windowTitle[i];
		if (ch < 0x80)
		{
			utf8Bytes.push_back(static_cast<byte_t>(ch));
		}
		else if (ch < 0x800)
		{
			utf8Bytes.push_back(static_cast<byte_t>(0xC0 | (ch >> 6)));
			utf8Bytes.push_back(static_cast<byte_t>(0x80 | (ch & 0x3F)));
		}
		else
		{
			utf8Bytes.push_back(static_cast<byte_t>(0xE0 | (ch >> 12)));
			utf8Bytes.push_back(static_cast<byte_t>(0x80 | ((ch >> 6) & 0x3F)));
			utf8Bytes.push_back(static_cast<byte_t>(0x80 | (ch & 0x3F)));
		}
	}
	out.writeShort(static_cast<short_t>(utf8Bytes.size()));
	for (byte_t b : utf8Bytes)
	{
		out.writeByte(b);
	}
	
	// var1.writeByte(this.slotsCount);
	out.writeByte(static_cast<byte_t>(this->slotsCount));
}

void Packet100OpenWindow::processPacket(NetHandler* handler)
{
	// Java: var1.func_20087_a(this);
	handler->func_20087_a(this);
}

int Packet100OpenWindow::getPacketSize()
{
	// Java: variable size
	// byte (1) + byte (1) + UTF string (2 + bytes) + byte (1)
	// Approximate - actual depends on string length
	return 4 + static_cast<int>(windowTitle.length());
}

int Packet100OpenWindow::getPacketId() const
{
	return 100;
}
