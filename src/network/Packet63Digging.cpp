#include "network/Packet63Digging.h"
#include "network/NetHandler.h"
#include <chrono>

Packet63Digging::Packet63Digging()
	: x(0)
	, y(0)
	, z(0)
	, face(0)
	, progress(0.0f)
	, timestamp(0)
{
}

void Packet63Digging::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.x = var1.readInt();
	this->x = in.readInt();
	
	// this.y = var1.readInt();
	this->y = in.readInt();
	
	// this.z = var1.readInt();
	this->z = in.readInt();
	
	// this.face = var1.readByte();
	this->face = in.readByte();
	
	// this.progress = var1.readFloat();
	this->progress = in.readFloat();
	
	// this.timestamp = System.currentTimeMillis();
	// Note: timestamp is set on read, not from packet
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	this->timestamp = static_cast<long_t>(ms.count());
}

void Packet63Digging::writePacketData(SocketOutputStream& out)
{
	// Java: writePacketData is empty (server-to-client only)
	// Packet63Digging is only sent from server to client
}

void Packet63Digging::processPacket(NetHandler* handler)
{
	// Java: var1.handle63Digging(this);
	handler->handle63Digging(this);
}

int Packet63Digging::getPacketSize()
{
	// Java: return 17;
	// int (4) + int (4) + int (4) + byte (1) + float (4) = 17
	return 17;
}

int Packet63Digging::getPacketId() const
{
	return 63;
}
