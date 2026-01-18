#include "network/Packet33RelEntityMoveLook.h"
#include "network/NetHandler.h"
#include "network/Packet30Entity.h"

Packet33RelEntityMoveLook::Packet33RelEntityMoveLook()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, yaw(0)
	, pitch(0)
{
}

void Packet33RelEntityMoveLook::readPacketData(SocketInputStream& in)
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
	
	// this.yaw = var1.readByte();
	this->yaw = in.readByte();
	
	// this.pitch = var1.readByte();
	this->pitch = in.readByte();
}

void Packet33RelEntityMoveLook::writePacketData(SocketOutputStream& out)
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
	
	// var1.writeByte(this.yaw);
	out.writeByte(this->yaw);
	
	// var1.writeByte(this.pitch);
	out.writeByte(this->pitch);
}

void Packet33RelEntityMoveLook::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntity(this);  // Uses Packet30Entity handler
	// Note: In Java this extends Packet30Entity, so handleEntity receives the full packet with movement and rotation data
	// In C++, we create a Packet30Entity and populate it with all the data
	Packet30Entity tempEntity;
	tempEntity.entityId = this->entityId;
	tempEntity.xPosition = this->xPosition;  // Relative movement as byte
	tempEntity.yPosition = this->yPosition;
	tempEntity.zPosition = this->zPosition;
	tempEntity.yaw = this->yaw;  // Rotation as byte
	tempEntity.pitch = this->pitch;
	tempEntity.rotating = true;  // Has rotation
	handler->handleEntity(&tempEntity);
}

int Packet33RelEntityMoveLook::getPacketSize()
{
	// Java: return 9;
	// int (4) + byte (1) + byte (1) + byte (1) + byte (1) + byte (1) = 9
	return 9;
}

int Packet33RelEntityMoveLook::getPacketId() const
{
	return 33;
}
