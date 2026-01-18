#include "network/Packet9.h"
#include "network/NetHandler.h"

Packet9::Packet9()
	: field_28048_a(0)
	, seed(0)
{
}

Packet9::Packet9(byte_t dimension, long_t seed)
	: field_28048_a(dimension)
	, seed(seed)
{
}

void Packet9::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_28048_a = var1.readByte();
	this->field_28048_a = in.readByte();
	
	// this.seed = var1.readLong();
	this->seed = in.readLong();
}

void Packet9::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.field_28048_a);
	out.writeByte(this->field_28048_a);
	
	// var1.writeLong(this.seed);
	out.writeLong(this->seed);
}

void Packet9::processPacket(NetHandler* handler)
{
	// Java: var1.func_9448_a(this);
	handler->func_9448_a(this);
}

int Packet9::getPacketSize()
{
	// Java: return 9;
	// byte (1) + long (8) = 9
	return 9;
}

int Packet9::getPacketId() const
{
	return 9;
}
