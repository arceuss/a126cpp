#include "network/Packet70Bed.h"
#include "network/NetHandler.h"

Packet70Bed::Packet70Bed()
	: field_25019_b(0)
{
}

void Packet70Bed::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_25019_b = var1.readByte();
	this->field_25019_b = static_cast<int_t>(in.read() & 0xFF);
}

void Packet70Bed::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.field_25019_b);
	out.writeByte(static_cast<byte_t>(this->field_25019_b));
}

void Packet70Bed::processPacket(NetHandler* handler)
{
	// Java: var1.handleBed(this);
	handler->handleBed(this);
}

int Packet70Bed::getPacketSize()
{
	// Java: return 1;
	// byte (1) = 1
	return 1;
}

int Packet70Bed::getPacketId() const
{
	return 70;
}
