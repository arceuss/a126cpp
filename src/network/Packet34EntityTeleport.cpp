#include "network/Packet34EntityTeleport.h"
#include "network/NetHandler.h"

Packet34EntityTeleport::Packet34EntityTeleport()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, yaw(0)
	, pitch(0)
{
}

void Packet34EntityTeleport::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.yaw = (byte)var1.read();
	this->yaw = static_cast<byte_t>(in.read() & 0xFF);
	
	// this.pitch = (byte)var1.read();
	this->pitch = static_cast<byte_t>(in.read() & 0xFF);
}

void Packet34EntityTeleport::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.write(this.yaw);
	out.writeByte(this->yaw);
	
	// var1.write(this.pitch);
	out.writeByte(this->pitch);
}

void Packet34EntityTeleport::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntityTeleport(this);
	handler->handleEntityTeleport(this);
}

int Packet34EntityTeleport::getPacketSize()
{
	// Java: return 34;
	// int (4) + int (4) + int (4) + int (4) + byte (1) + byte (1) = 18
	// But Java says 34. The calculation might be approximate or include overhead.
	// Let's match Java exactly:
	return 34;
}

int Packet34EntityTeleport::getPacketId() const
{
	return 34;
}
