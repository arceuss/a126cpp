#include "network/Packet19EntityAction.h"
#include "network/NetHandler.h"

Packet19EntityAction::Packet19EntityAction()
	: entityId(0)
	, state(0)
{
}

Packet19EntityAction::Packet19EntityAction(int_t entityId, int_t state)
	: entityId(entityId)
	, state(state)
{
}

void Packet19EntityAction::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.state = var1.readByte();
	this->state = static_cast<int_t>(in.read() & 0xFF);
}

void Packet19EntityAction::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeByte(this.state);
	out.writeByte(static_cast<byte_t>(this->state));
}

void Packet19EntityAction::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntityAction(this);
	handler->handleEntityAction(this);
}

int Packet19EntityAction::getPacketSize()
{
	// Java: return 5;
	// int (4) + byte (1) = 5
	return 5;
}

int Packet19EntityAction::getPacketId() const
{
	return 19;
}
