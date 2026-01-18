#include "network/Packet22Collect.h"
#include "network/NetHandler.h"

Packet22Collect::Packet22Collect()
	: collectedEntityId(0)
	, collectorEntityId(0)
{
}

Packet22Collect::Packet22Collect(int_t collectedEntityId, int_t collectorEntityId)
	: collectedEntityId(collectedEntityId)
	, collectorEntityId(collectorEntityId)
{
}

void Packet22Collect::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.collectedEntityId = var1.readInt();
	this->collectedEntityId = in.readInt();
	
	// this.collectorEntityId = var1.readInt();
	this->collectorEntityId = in.readInt();
}

void Packet22Collect::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.collectedEntityId);
	out.writeInt(this->collectedEntityId);
	
	// var1.writeInt(this.collectorEntityId);
	out.writeInt(this->collectorEntityId);
}

void Packet22Collect::processPacket(NetHandler* handler)
{
	// Java: var1.handleCollect(this);
	handler->handleCollect(this);
}

int Packet22Collect::getPacketSize()
{
	// Java: return 8;
	// int (4) + int (4) = 8
	return 8;
}

int Packet22Collect::getPacketId() const
{
	return 22;
}
