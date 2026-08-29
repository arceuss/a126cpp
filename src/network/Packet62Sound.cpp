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
	const ushort_t utfByteLength = static_cast<ushort_t>(in.readShort());
	std::vector<byte_t> utfBytes(utfByteLength);
	in.readFully(utfBytes.data(), utfBytes.size());

	sound.clear();
	sound.reserve(utfByteLength);
	size_t index = 0;
	while (index < utfBytes.size())
	{
		const ubyte_t first = static_cast<ubyte_t>(utfBytes[index]);
		switch (first >> 4)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			++index;
			sound.push_back(static_cast<char16_t>(first));
			break;

		case 12:
		case 13:
		{
			index += 2;
			if (index > utfBytes.size())
				throw std::runtime_error("Malformed modified UTF-8 input");
			const ubyte_t second = static_cast<ubyte_t>(utfBytes[index - 1]);
			if ((second & 0xC0) != 0x80)
				throw std::runtime_error("Malformed modified UTF-8 input");
			sound.push_back(static_cast<char16_t>(
				((first & 0x1F) << 6) | (second & 0x3F)));
			break;
		}

		case 14:
		{
			index += 3;
			if (index > utfBytes.size())
				throw std::runtime_error("Malformed modified UTF-8 input");
			const ubyte_t second = static_cast<ubyte_t>(utfBytes[index - 2]);
			const ubyte_t third = static_cast<ubyte_t>(utfBytes[index - 1]);
			if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80)
				throw std::runtime_error("Malformed modified UTF-8 input");
			sound.push_back(static_cast<char16_t>(
				((first & 0x0F) << 12) | ((second & 0x3F) << 6)
				| (third & 0x3F)));
			break;
		}

		default:
			throw std::runtime_error("Malformed modified UTF-8 input");
		}
	}

	locX = in.readDouble();
	locY = in.readDouble();
	locZ = in.readDouble();
	f = in.readFloat();
	f1 = in.readFloat();
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
	return static_cast<int>(sound.length()) + 24 + 8;
}

int Packet62Sound::getPacketId() const
{
	return 62;
}
