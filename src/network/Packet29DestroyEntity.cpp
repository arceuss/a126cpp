#include "network/Packet29DestroyEntity.h"
#include "network/NetHandler.h"

Packet29DestroyEntity::Packet29DestroyEntity()
	: entityId(0)
{
}

void Packet29DestroyEntity::readPacketData(SocketInputStream& in)
{
	// Java: this.entityId = var1.readInt();
	this->entityId = in.readInt();
}

void Packet29DestroyEntity::writePacketData(SocketOutputStream& out)
{
	// Java: var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
}

void Packet29DestroyEntity::processPacket(NetHandler* handler)
{
	// Java: var1.handleDestroyEntity(this);
	handler->handleDestroyEntity(this);
}

int Packet29DestroyEntity::getPacketSize()
{
	// Java: return 4;
	return 4;
}

int Packet29DestroyEntity::getPacketId() const
{
	return 29;
}
