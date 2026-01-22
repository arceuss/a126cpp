#include "network/Packet4UpdateTime.h"
#include "network/NetHandler.h"

Packet4UpdateTime::Packet4UpdateTime()
	: time(0)
{
}

void Packet4UpdateTime::readPacketData(SocketInputStream& in)
{
	// Java: this.time = var1.readLong();
	this->time = in.readLong();
}

void Packet4UpdateTime::writePacketData(SocketOutputStream& out)
{
	// Java: var1.writeLong(this.time);
	out.writeLong(this->time);
}

void Packet4UpdateTime::processPacket(NetHandler* handler)
{
	// Java: var1.handleUpdateTime(this);
	handler->handleUpdateTime(this);
}

int Packet4UpdateTime::getPacketSize()
{
	// Java: return 8;
	return 8;
}

int Packet4UpdateTime::getPacketId() const
{
	return 4;
}
