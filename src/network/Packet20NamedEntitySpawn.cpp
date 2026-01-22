#include "network/Packet20NamedEntitySpawn.h"
#include "network/NetHandler.h"

Packet20NamedEntitySpawn::Packet20NamedEntitySpawn()
	: entityId(0)
	, name(u"")
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, rotation(0)
	, pitch(0)
	, currentItem(0)
{
}

void Packet20NamedEntitySpawn::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.name = readString(var1, 16);
	this->name = Packet::readString(in, 16);
	
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
	
	// this.currentItem = var1.readShort();
	this->currentItem = in.readShort();
}

void Packet20NamedEntitySpawn::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// writeString(this.name, var1);
	Packet::writeString(this->name, out);
	
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
	
	// var1.writeShort(this.currentItem);
	out.writeShort(this->currentItem);
}

void Packet20NamedEntitySpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handleNamedEntitySpawn(this);
	handler->handleNamedEntitySpawn(this);
}

int Packet20NamedEntitySpawn::getPacketSize()
{
	// Java: return 28;
	// int (4) + string (2 + name.length()*2) + int (4) + int (4) + int (4) + byte (1) + byte (1) + short (2)
	// Actually, let's calculate: 4 + (2 + name.length()*2) + 4 + 4 + 4 + 1 + 1 + 2 = 22 + name.length()*2
	// But Java says 28. This might assume a fixed string length or the calculation is approximate.
	// Let's match Java exactly:
	return 28;
}

int Packet20NamedEntitySpawn::getPacketId() const
{
	return 20;
}
