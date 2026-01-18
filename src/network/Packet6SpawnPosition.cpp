#include "network/Packet6SpawnPosition.h"
#include "network/NetHandler.h"

Packet6SpawnPosition::Packet6SpawnPosition()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
{
}

void Packet6SpawnPosition::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
}

void Packet6SpawnPosition::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
}

void Packet6SpawnPosition::processPacket(NetHandler* handler)
{
	// Java: var1.handleSpawnPosition(this);
	handler->handleSpawnPosition(this);
}

int Packet6SpawnPosition::getPacketSize()
{
	// Java: return 12;
	// int (4) + int (4) + int (4) = 12
	return 12;
}

int Packet6SpawnPosition::getPacketId() const
{
	return 6;
}
