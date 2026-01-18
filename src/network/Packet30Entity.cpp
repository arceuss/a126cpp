#include "network/Packet30Entity.h"
#include "network/NetHandler.h"

Packet30Entity::Packet30Entity()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, yaw(0)
	, pitch(0)
	, rotating(false)
{
}

void Packet30Entity::readPacketData(SocketInputStream& in)
{
	// Java: this.entityId = var1.readInt();
	// Note: Java only reads entityId, but packet has other fields that may be used elsewhere
	this->entityId = in.readInt();
}

void Packet30Entity::writePacketData(SocketOutputStream& out)
{
	// Java: var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
}

void Packet30Entity::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntity(this);
	handler->handleEntity(this);
}

int Packet30Entity::getPacketSize()
{
	// Java: return 4;
	return 4;
}

int Packet30Entity::getPacketId() const
{
	return 30;
}
