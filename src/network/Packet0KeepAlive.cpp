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
	// Java: Alpha 1.2.6 - empty method, does nothing
	// Note: The client may need to respond to keep-alive packets to prevent timeout,
	// but this is handled elsewhere (e.g., the client may send keep-alive packets proactively
	// or the server may accept other packets as keep-alive responses)
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
