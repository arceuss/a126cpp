#include "network/Packet7.h"
#include "network/NetHandler.h"

Packet7::Packet7()
	: field_9277_a(0)
	, field_9276_b(0)
	, field_9278_c(0)
{
}

Packet7::Packet7(int_t user, int_t target, byte_t leftClick)
	: field_9277_a(user)
	, field_9276_b(target)
	, field_9278_c(leftClick)
{
}

void Packet7::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_9277_a = var1.readInt();
	this->field_9277_a = in.readInt();
	
	// this.field_9276_b = var1.readInt();
	this->field_9276_b = in.readInt();
	
	// this.field_9278_c = var1.readByte();
	this->field_9278_c = static_cast<byte_t>(in.read() & 0xFF);
}

void Packet7::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_9277_a);
	out.writeInt(this->field_9277_a);
	
	// var1.writeInt(this.field_9276_b);
	out.writeInt(this->field_9276_b);
	
	// var1.writeByte(this.field_9278_c);
	out.writeByte(this->field_9278_c);
}

void Packet7::processPacket(NetHandler* handler)
{
	// Java: var1.func_6499_a(this);
	handler->func_6499_a(this);
}

int Packet7::getPacketSize()
{
	// Java: return 9;
	// int (4) + int (4) + byte (1) = 9
	return 9;
}

int Packet7::getPacketId() const
{
	return 7;
}
