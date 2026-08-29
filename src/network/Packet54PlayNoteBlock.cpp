#include "network/Packet54PlayNoteBlock.h"
#include "network/NetHandler.h"

Packet54PlayNoteBlock::Packet54PlayNoteBlock()
	: xLocation(0)
	, yLocation(0)
	, zLocation(0)
	, instrumentType(0)
	, pitch(0)
{
}

void Packet54PlayNoteBlock::readPacketData(SocketInputStream& in)
{
	xLocation = in.readInt();
	yLocation = in.readShort();
	zLocation = in.readInt();
	instrumentType = in.read();
	pitch = in.read();
}

void Packet54PlayNoteBlock::writePacketData(SocketOutputStream& out)
{
	out.writeInt(xLocation);
	out.writeShort(static_cast<short_t>(yLocation));
	out.writeInt(zLocation);
	out.write(instrumentType);
	out.write(pitch);
}

void Packet54PlayNoteBlock::processPacket(NetHandler* handler)
{
	// Java: var1.handleNoteBlock(this);
	handler->handleNoteBlock(this);
}

int Packet54PlayNoteBlock::getPacketSize()
{
	// Java: return 12;
	// int (4) + short (2) + int (4) + byte (1) + byte (1) = 12
	return 12;
}

int Packet54PlayNoteBlock::getPacketId() const
{
	return 54;
}
