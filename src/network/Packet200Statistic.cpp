#include "network/Packet200Statistic.h"
#include "network/NetHandler.h"

Packet200Statistic::Packet200Statistic()
	: field_27052_a(0)
	, field_27051_b(0)
{
}

void Packet200Statistic::readPacketData(SocketInputStream& in)
{
	this->field_27052_a = in.readInt();
	this->field_27051_b = in.readByte();
}

void Packet200Statistic::writePacketData(SocketOutputStream& out)
{
	out.writeInt(this->field_27052_a);
	out.writeByte(static_cast<byte_t>(this->field_27051_b));
}

void Packet200Statistic::processPacket(NetHandler* handler)
{
	// Alpha Packet200Statistic.java:18-19.
	handler->handleStatistic(this);
}

int Packet200Statistic::getPacketSize()
{
	// Alpha returns 6 even though the serialized payload is five bytes.
	return 6;
}

int Packet200Statistic::getPacketId() const
{
	return 200;
}
