#include "network/Packet130UpdateSign.h"
#include "network/NetHandler.h"

Packet130UpdateSign::Packet130UpdateSign()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
{
	isChunkDataPacket = true;
}

Packet130UpdateSign::Packet130UpdateSign(int_t xPosition, int_t yPosition, int_t zPosition,
	const jstring (&signLines)[4])
	: xPosition(xPosition)
	, yPosition(yPosition)
	, zPosition(zPosition)
{
	isChunkDataPacket = true;
	for (int_t i = 0; i < 4; ++i)
		this->signLines[i] = signLines[i];
}

void Packet130UpdateSign::readPacketData(SocketInputStream& in)
{
	this->xPosition = in.readInt();
	this->yPosition = in.readShort();
	this->zPosition = in.readInt();
	for (int i = 0; i < 4; ++i)
	{
		this->signLines[i] = Packet::readString(in, 15);
	}
}

void Packet130UpdateSign::writePacketData(SocketOutputStream& out)
{
	out.writeInt(this->xPosition);
	out.writeShort(static_cast<short_t>(this->yPosition));
	out.writeInt(this->zPosition);
	for (int i = 0; i < 4; ++i)
	{
		Packet::writeString(this->signLines[i], out);
	}
}

void Packet130UpdateSign::processPacket(NetHandler* handler)
{
	// Alpha Packet130UpdateSign.java:53-54.
	handler->handleSignUpdate(this);
}

int Packet130UpdateSign::getPacketSize()
{
	int_t size = 0;
	for (int_t i = 0; i < 4; ++i)
		size += static_cast<int_t>(signLines[i].length());
	return size;
}

int Packet130UpdateSign::getPacketId() const
{
	return 130;
}
