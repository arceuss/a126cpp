#include "network/Packet100OpenWindow.h"
#include "network/NetHandler.h"
#include <vector>
#include <stdexcept>

jstring Packet100OpenWindow::readUTF(SocketInputStream& in)
{
	// DataInputStream.readUTF reads an unsigned-short byte count.
	const int_t byteLength = static_cast<int_t>(static_cast<ushort_t>(in.readShort()));
	std::vector<byte_t> bytes(static_cast<size_t>(byteLength));
	in.readFully(bytes.data(), bytes.size());

	jstring result;
	result.reserve(static_cast<size_t>(byteLength));
	for (int_t i = 0; i < byteLength;)
	{
		const ubyte_t b1 = static_cast<ubyte_t>(bytes[static_cast<size_t>(i++)]);
		if ((b1 & 0x80) == 0)
		{
			result += static_cast<char16_t>(b1);
		}
		else if ((b1 & 0xE0) == 0xC0)
		{
			if (i >= byteLength)
				throw std::runtime_error("UTFDataFormatException: malformed input: partial character at end");
			const ubyte_t b2 = static_cast<ubyte_t>(bytes[static_cast<size_t>(i++)]);
			if ((b2 & 0xC0) != 0x80)
				throw std::runtime_error("UTFDataFormatException: malformed input around byte " + std::to_string(i));
			result += static_cast<char16_t>(((b1 & 0x1F) << 6) | (b2 & 0x3F));
		}
		else if ((b1 & 0xF0) == 0xE0)
		{
			if (i + 1 >= byteLength)
				throw std::runtime_error("UTFDataFormatException: malformed input: partial character at end");
			const ubyte_t b2 = static_cast<ubyte_t>(bytes[static_cast<size_t>(i++)]);
			const ubyte_t b3 = static_cast<ubyte_t>(bytes[static_cast<size_t>(i++)]);
			if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
				throw std::runtime_error("UTFDataFormatException: malformed input around byte " + std::to_string(i - 1));
			result += static_cast<char16_t>(
				((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));
		}
		else
		{
			throw std::runtime_error("UTFDataFormatException: malformed input around byte " + std::to_string(i));
		}
	}
	return result;
}

void Packet100OpenWindow::writeUTF(const jstring& value, SocketOutputStream& out)
{
	size_t byteLength = 0;
	for (char16_t c : value)
	{
		byteLength += c == 0 ? 2 : c < 0x80 ? 1 : c < 0x800 ? 2 : 3;
		if (byteLength > 65535)
			throw std::runtime_error("UTFDataFormatException: encoded string too long");
	}

	out.writeShort(static_cast<short_t>(byteLength));
	for (char16_t c : value)
	{
		if (c == 0)
		{
			out.writeByte(static_cast<byte_t>(0xC0));
			out.writeByte(static_cast<byte_t>(0x80));
		}
		else if (c < 0x80)
		{
			out.writeByte(static_cast<byte_t>(c));
		}
		else if (c < 0x800)
		{
			out.writeByte(static_cast<byte_t>(0xC0 | (c >> 6)));
			out.writeByte(static_cast<byte_t>(0x80 | (c & 0x3F)));
		}
		else
		{
			out.writeByte(static_cast<byte_t>(0xE0 | (c >> 12)));
			out.writeByte(static_cast<byte_t>(0x80 | ((c >> 6) & 0x3F)));
			out.writeByte(static_cast<byte_t>(0x80 | (c & 0x3F)));
		}
	}
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
	this->windowId = static_cast<int_t>(in.readByte());
	
	// this.inventoryType = var1.readByte();
	this->inventoryType = static_cast<int_t>(in.readByte());
	
	// this.windowTitle = var1.readUTF();
	this->windowTitle = readUTF(in);
	
	// this.slotsCount = var1.readByte();
	this->slotsCount = static_cast<int_t>(in.readByte());
}

void Packet100OpenWindow::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
	
	// var1.writeByte(this.inventoryType);
	out.writeByte(static_cast<byte_t>(this->inventoryType));
	
	// var1.writeUTF(this.windowTitle);
	writeUTF(this->windowTitle, out);
	
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
	// Java: return 3 + this.windowTitle.length();
	return static_cast<int_t>(3U + static_cast<uint_t>(windowTitle.length()));
}

int Packet100OpenWindow::getPacketId() const
{
	return 100;
}
