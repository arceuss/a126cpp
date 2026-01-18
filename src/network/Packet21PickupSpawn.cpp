#include "network/Packet21PickupSpawn.h"
#include "network/NetHandler.h"

Packet21PickupSpawn::Packet21PickupSpawn()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, rotation(0)
	, pitch(0)
	, roll(0)
	, itemId(0)
	, count(0)
	, itemDamage(0)
{
}

void Packet21PickupSpawn::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.itemId = var1.readShort();
	this->itemId = in.readShort();
	
	// this.count = var1.readByte();
	this->count = in.readByte();
	
	// this.itemDamage = var1.readShort();
	this->itemDamage = in.readShort();
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.rotation = var1.readByte();
	this->rotation = in.readByte();
	
	// this.pitch = var1.readByte();
	this->pitch = in.readByte();
	
	// this.roll = var1.readByte();
	this->roll = in.readByte();
}

void Packet21PickupSpawn::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeShort(this.itemId);
	out.writeShort(this->itemId);
	
	// var1.writeByte(this.count);
	out.writeByte(this->count);
	
	// var1.writeShort(this.itemDamage);
	out.writeShort(this->itemDamage);
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.writeByte(this.rotation);
	out.writeByte(this->rotation);
	
	// var1.writeByte(this.pitch);
	out.writeByte(this->pitch);
	
	// var1.writeByte(this.roll);
	out.writeByte(this->roll);
}

void Packet21PickupSpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handlePickupSpawn(this);
	handler->handlePickupSpawn(this);
}

int Packet21PickupSpawn::getPacketSize()
{
	// Java: return 24;
	// int (4) + short (2) + byte (1) + short (2) + int (4) + int (4) + int (4) + byte (1) + byte (1) + byte (1) = 26
	// But Java says 24. The calculation might be off. Let's match Java exactly:
	return 24;
}

int Packet21PickupSpawn::getPacketId() const
{
	return 21;
}
