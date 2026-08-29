#include "network/Packet0KeepAlive.h"
#include "network/NetHandler.h"

Packet0KeepAlive::Packet0KeepAlive()
{
}

void Packet0KeepAlive::readPacketData(SocketInputStream& in)
{
	// Java: empty - no data
}

void Packet0KeepAlive::writePacketData(SocketOutputStream& out)
{
	// Java: empty - no data
}

void Packet0KeepAlive::processPacket(NetHandler* handler)
{
	// Java: empty method.
}

int Packet0KeepAlive::getPacketSize()
{
	// Java: return 0;
	return 0;
}

int Packet0KeepAlive::getPacketId() const
{
	return 0;
}
