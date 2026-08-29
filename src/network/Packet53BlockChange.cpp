#include "network/Packet53BlockChange.h"
#include "network/NetHandler.h"

Packet53BlockChange::Packet53BlockChange()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, type(0)
	, metadata(0)
{
	// Java: this.isChunkDataPacket = true;
	this->isChunkDataPacket = true;
}

void Packet53BlockChange::readPacketData(SocketInputStream& in)
{
	xPosition = in.readInt();
	yPosition = in.read();
	zPosition = in.readInt();
	type = in.read();
	metadata = in.read();
}

void Packet53BlockChange::writePacketData(SocketOutputStream& out)
{
	out.writeInt(xPosition);
	out.write(yPosition);
	out.writeInt(zPosition);
	out.write(type);
	out.write(metadata);
}

void Packet53BlockChange::processPacket(NetHandler* handler)
{
	// Java: var1.handleBlockChange(this);
	handler->handleBlockChange(this);
}

int Packet53BlockChange::getPacketSize()
{
	// Java: return 11;
	// int (4) + byte (1) + int (4) + byte (1) + byte (1) = 11
	return 11;
}

int Packet53BlockChange::getPacketId() const
{
	return 53;
}
