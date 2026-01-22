#include "network/Packet31RelEntityMove.h"
#include "network/NetHandler.h"
#include "network/Packet30Entity.h"

Packet31RelEntityMove::Packet31RelEntityMove()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
{
}

void Packet31RelEntityMove::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// super.readPacketData(var1);  // Reads entityId
	this->entityId = in.readInt();
	
	// this.xPosition = var1.readByte();
	this->xPosition = in.readByte();
	
	// this.yPosition = var1.readByte();
	this->yPosition = in.readByte();
	
	// this.zPosition = var1.readByte();
	this->zPosition = in.readByte();
}

void Packet31RelEntityMove::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// super.writePacketData(var1);  // Writes entityId
	out.writeInt(this->entityId);
	
	// var1.writeByte(this.xPosition);
	out.writeByte(this->xPosition);
	
	// var1.writeByte(this.yPosition);
	out.writeByte(this->yPosition);
	
	// var1.writeByte(this.zPosition);
	out.writeByte(this->zPosition);
}

void Packet31RelEntityMove::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntity(this);  // Uses Packet30Entity handler
	// Note: In Java this extends Packet30Entity, so handleEntity receives the full packet with movement data
	// In C++, we create a Packet30Entity and populate it with the movement data
	Packet30Entity tempEntity;
	tempEntity.entityId = this->entityId;
	tempEntity.xPosition = this->xPosition;  // Relative movement as byte
	tempEntity.yPosition = this->yPosition;
	tempEntity.zPosition = this->zPosition;
	tempEntity.rotating = false;  // No rotation in Packet31RelEntityMove
	handler->handleEntity(&tempEntity);
}

int Packet31RelEntityMove::getPacketSize()
{
	// Java: return 7;
	// int (4) + byte (1) + byte (1) + byte (1) = 7
	return 7;
}

int Packet31RelEntityMove::getPacketId() const
{
	return 31;
}
