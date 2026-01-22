#include "network/Packet8.h"
#include "network/NetHandler.h"

Packet8::Packet8()
	: healthMP(0)
{
}

void Packet8::readPacketData(SocketInputStream& in)
{
	// Java: this.healthMP = var1.readShort();
	this->healthMP = in.readShort();
}

void Packet8::writePacketData(SocketOutputStream& out)
{
	// Java: var1.writeShort(this.healthMP);
	out.writeShort(static_cast<short_t>(this->healthMP));
}

void Packet8::processPacket(NetHandler* handler)
{
	// Java: var1.handleHealth(this);
	handler->handleHealth(this);
}

int Packet8::getPacketSize()
{
	// Java: return 2;
	return 2;
}

int Packet8::getPacketId() const
{
	return 8;
}
