#include "network/Packet39.h"
#include "network/NetHandler.h"

// Beta 1.2: SetRidingPacket constructor - matches newb12 SetRidingPacket.java:12-13
Packet39::Packet39()
	: riderId(0)
	, riddenId(0)
{
}

// Beta 1.2: SetRidingPacket.read() - matches newb12 SetRidingPacket.java:26-28 exactly
void Packet39::readPacketData(SocketInputStream& in)
{
	// Beta: this.riderId = var1.readInt();
	this->riderId = in.readInt();
	
	// Beta: this.riddenId = var1.readInt();
	this->riddenId = in.readInt();
}

// Beta 1.2: SetRidingPacket.write() - matches newb12 SetRidingPacket.java:32-34 exactly
void Packet39::writePacketData(SocketOutputStream& out)
{
	// Beta: var1.writeInt(this.riderId);
	out.writeInt(this->riderId);
	
	// Beta: var1.writeInt(this.riddenId);
	out.writeInt(this->riddenId);
}

// Beta 1.2: SetRidingPacket.handle() - matches newb12 SetRidingPacket.java:38-40
void Packet39::processPacket(NetHandler* handler)
{
	// Beta: var1.handleRidePacket(this);
	// In our codebase, this maps to func_6497_a() for compatibility
	handler->func_6497_a(this);
}

// Beta 1.2: SetRidingPacket.getEstimatedSize() - matches newb12 SetRidingPacket.java:21-23
int Packet39::getPacketSize()
{
	// Beta: return 8;
	// int (4) + int (4) = 8
	return 8;
}

int Packet39::getPacketId() const
{
	return 39;
}
