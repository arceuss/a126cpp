#include "network/Packet71Weather.h"
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

int_t javaFloorDouble(double value)
{
	const int_t truncated = javaDoubleToInt(value);
	if (value >= static_cast<double>(truncated))
		return truncated;
	if (truncated == (std::numeric_limits<int_t>::min)())
		return (std::numeric_limits<int_t>::max)();
	return truncated - 1;
}
}


Packet71Weather::Packet71Weather()
	: field_27054_a(0)
	, field_27053_b(0)
	, field_27057_c(0)
	, field_27056_d(0)
	, field_27055_e(0)
{
}
Packet71Weather::Packet71Weather(const Entity& entity)
	: field_27054_a(entity.entityId)
	, field_27053_b(javaFloorDouble(entity.x * 32.0))
	, field_27057_c(javaFloorDouble(entity.y * 32.0))
	, field_27056_d(javaFloorDouble(entity.z * 32.0))
	, field_27055_e(0)
{
}


void Packet71Weather::readPacketData(SocketInputStream& in)
{
	field_27054_a = in.readInt();
	field_27055_e = in.readByte();
	field_27053_b = in.readInt();
	field_27057_c = in.readInt();
	field_27056_d = in.readInt();
}

void Packet71Weather::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_27054_a);
	out.writeInt(this->field_27054_a);
	
	// var1.writeByte(this.field_27055_e);
	out.writeByte(static_cast<byte_t>(this->field_27055_e));
	
	// var1.writeInt(this.field_27053_b);
	out.writeInt(this->field_27053_b);
	
	// var1.writeInt(this.field_27057_c);
	out.writeInt(this->field_27057_c);
	
	// var1.writeInt(this.field_27056_d);
	out.writeInt(this->field_27056_d);
}

void Packet71Weather::processPacket(NetHandler* handler)
{
	// Java: var1.handleWeather(this);
	handler->handleWeather(this);
}

int Packet71Weather::getPacketSize()
{
	// Java: return 17;
	// int (4) + byte (1) + int (4) + int (4) + int (4) = 17
	return 17;
}

int Packet71Weather::getPacketId() const
{
	return 71;
}
