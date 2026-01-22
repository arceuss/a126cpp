#include "network/Packet38.h"
#include "network/NetHandler.h"

Packet38::Packet38()
	: field_9274_a(0)
	, field_9273_b(0)
{
}

void Packet38::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_9274_a = var1.readInt();
	this->field_9274_a = in.readInt();
	
	// this.field_9273_b = var1.readByte();
	this->field_9273_b = in.readByte();
}

void Packet38::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_9274_a);
	out.writeInt(this->field_9274_a);
	
	// var1.writeByte(this.field_9273_b);
	out.writeByte(this->field_9273_b);
}

void Packet38::processPacket(NetHandler* handler)
{
	// Java: var1.func_9447_a(this);
	handler->func_9447_a(this);
}

int Packet38::getPacketSize()
{
	// Java: return 5;
	// int (4) + byte (1) = 5
	return 5;
}

int Packet38::getPacketId() const
{
	return 38;
}
