#include "network/Packet200Statistic.h"
#include "network/NetHandler.h"

Packet200Statistic::Packet200Statistic()
	: field_27052_a(0)
	, field_27051_b(0)
{
}

void Packet200Statistic::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_27052_a = var1.readInt();
	this->field_27052_a = in.readInt();
	
	// this.field_27051_b = var1.readByte();
	this->field_27051_b = static_cast<int_t>(in.read() & 0xFF);
}

void Packet200Statistic::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_27052_a);
	out.writeInt(this->field_27052_a);
	
	// var1.writeByte(this.field_27051_b);
	out.writeByte(static_cast<byte_t>(this->field_27051_b));
}

void Packet200Statistic::processPacket(NetHandler* handler)
{
	// Java: var1.handleStatistic(this);
	handler->handleStatistic(this);
}

int Packet200Statistic::getPacketSize()
{
	// Java: return 5;
	// int (4) + byte (1) = 5
	return 5;
}

int Packet200Statistic::getPacketId() const
{
	return 200;
}
