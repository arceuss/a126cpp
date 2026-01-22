#include "network/Packet32EntityLook.h"
#include "network/NetHandler.h"
#include "network/Packet30Entity.h"

Packet32EntityLook::Packet32EntityLook()
	: entityId(0)
	, yaw(0)
	, pitch(0)
{
}

void Packet32EntityLook::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// super.readPacketData(var1);  // Reads entityId
	this->entityId = in.readInt();
	
	// this.yaw = var1.readByte();
	this->yaw = in.readByte();
	
	// this.pitch = var1.readByte();
	this->pitch = in.readByte();
}

void Packet32EntityLook::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// super.writePacketData(var1);  // Writes entityId
	out.writeInt(this->entityId);
	
	// var1.writeByte(this.yaw);
	out.writeByte(this->yaw);
	
	// var1.writeByte(this.pitch);
	out.writeByte(this->pitch);
}

void Packet32EntityLook::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntity(this);  // Uses Packet30Entity handler
	// Note: In Java this extends Packet30Entity, so handleEntity receives the full packet with rotation data
	// In C++, we create a Packet30Entity and populate it with the rotation data
	Packet30Entity tempEntity;
	tempEntity.entityId = this->entityId;
	tempEntity.xPosition = 0;  // No movement in Packet32EntityLook
	tempEntity.yPosition = 0;
	tempEntity.zPosition = 0;
	tempEntity.yaw = this->yaw;  // Rotation as byte
	tempEntity.pitch = this->pitch;
	tempEntity.rotating = true;  // Has rotation
	handler->handleEntity(&tempEntity);
}

int Packet32EntityLook::getPacketSize()
{
	// Java: return 6;
	// int (4) + byte (1) + byte (1) = 6
	return 6;
}

int Packet32EntityLook::getPacketId() const
{
	return 32;
}
