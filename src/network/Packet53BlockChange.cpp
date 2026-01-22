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
	// Java: EXACT ORDER
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.read();
	this->yPosition = static_cast<byte_t>(in.read() & 0xFF);
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.type = var1.read();
	this->type = static_cast<byte_t>(in.read() & 0xFF);
	
	// this.metadata = var1.read();
	this->metadata = static_cast<byte_t>(in.read() & 0xFF);
}

void Packet53BlockChange::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.write(this.yPosition);
	out.writeByte(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.write(this.type);
	out.writeByte(this->type);
	
	// var1.write(this.metadata);
	out.writeByte(this->metadata);
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
