#include "network/Packet50PreChunk.h"
#include "network/NetHandler.h"

Packet50PreChunk::Packet50PreChunk()
	: xPosition(0)
	, yPosition(0)
	, mode(false)
{
}

void Packet50PreChunk::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.mode = var1.read() != 0;
	int b = in.read();
	this->mode = (b != 0);
}

void Packet50PreChunk::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.write(this.mode ? 1 : 0);
	out.write(this->mode ? 1 : 0);
}

void Packet50PreChunk::processPacket(NetHandler* handler)
{
	// Java: var1.handlePreChunk(this);
	handler->handlePreChunk(this);
}

int Packet50PreChunk::getPacketSize()
{
	// Java: return 9;
	// int (4) + int (4) + byte (1) = 9
	return 9;
}

int Packet50PreChunk::getPacketId() const
{
	return 50;
}
