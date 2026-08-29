#include "network/Packet28.h"
#include "network/NetHandler.h"
#include "world/entity/Entity.h"
#include <cmath>
#include <limits>

namespace
{
int_t javaDoubleToInt(double value)
{
	if (std::isnan(value))
		return 0;
	if (value >= static_cast<double>((std::numeric_limits<int_t>::max)()))
		return (std::numeric_limits<int_t>::max)();
	if (value <= static_cast<double>((std::numeric_limits<int_t>::min)()))
		return (std::numeric_limits<int_t>::min)();
	return static_cast<int_t>(value);
}
}

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
	const double maxVelocity = 3.9;
	if (motionX < -maxVelocity)
		motionX = -maxVelocity;
	if (motionY < -maxVelocity)
		motionY = -maxVelocity;
	if (motionZ < -maxVelocity)
		motionZ = -maxVelocity;
	if (motionX > maxVelocity)
		motionX = maxVelocity;
	if (motionY > maxVelocity)
		motionY = maxVelocity;
	if (motionZ > maxVelocity)
		motionZ = maxVelocity;

	this->motionX = javaDoubleToInt(motionX * 8000.0);
	this->motionY = javaDoubleToInt(motionY * 8000.0);
	this->motionZ = javaDoubleToInt(motionZ * 8000.0);
}

Packet28::Packet28(Entity &entity)
	: Packet28(entity.entityId, entity.xd, entity.yd, entity.zd)
{
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
	out.writeShort(static_cast<short_t>(this->motionX));
	
	// var1.writeShort(this.field_6369_c);
	out.writeShort(static_cast<short_t>(this->motionY));
	
	// var1.writeShort(this.field_6368_d);
	out.writeShort(static_cast<short_t>(this->motionZ));
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
