#include "network/Packet28.h"
#include "network/NetHandler.h"
#include <algorithm>
#ifdef _WIN32
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#endif

Packet28::Packet28()
	: entityId(0)
	, motionX(0)
	, motionY(0)
	, motionZ(0)
{
}

Packet28::Packet28(int_t entityId, double motionX, double motionY, double motionZ)
	: entityId(entityId)
{
	// Java: double var8 = 3.9D;
	//       if(var2 < -var8) var2 = -var8;
	//       ... clamp to [-3.9, 3.9]
	double maxVel = 3.9;
	motionX = (std::max)(-maxVel, (std::min)(maxVel, motionX));
	motionY = (std::max)(-maxVel, (std::min)(maxVel, motionY));
	motionZ = (std::max)(-maxVel, (std::min)(maxVel, motionZ));
	
	// Java: this.field_6366_b = (int)(var2 * 8000.0D);
	this->motionX = static_cast<short_t>(static_cast<int_t>(motionX * 8000.0));
	this->motionY = static_cast<short_t>(static_cast<int_t>(motionY * 8000.0));
	this->motionZ = static_cast<short_t>(static_cast<int_t>(motionZ * 8000.0));
}

void Packet28::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_6367_a = var1.readInt();
	this->entityId = in.readInt();
	
	// this.field_6366_b = var1.readShort();
	this->motionX = in.readShort();
	
	// this.field_6369_c = var1.readShort();
	this->motionY = in.readShort();
	
	// this.field_6368_d = var1.readShort();
	this->motionZ = in.readShort();
}

void Packet28::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_6367_a);
	out.writeInt(this->entityId);
	
	// var1.writeShort(this.field_6366_b);
	out.writeShort(this->motionX);
	
	// var1.writeShort(this.field_6369_c);
	out.writeShort(this->motionY);
	
	// var1.writeShort(this.field_6368_d);
	out.writeShort(this->motionZ);
}

void Packet28::processPacket(NetHandler* handler)
{
	// Java: var1.func_6498_a(this);
	handler->func_6498_a(this);
}

int Packet28::getPacketSize()
{
	// Java: return 10;
	// int (4) + short (2) + short (2) + short (2) = 10
	return 10;
}

int Packet28::getPacketId() const
{
	return 28;
}
