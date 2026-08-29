#include "network/Packet39.h"
#include "network/NetHandler.h"

Packet39::Packet39()
	: field_6365_a(0)
	, field_6364_b(0)
{
}

void Packet39::readPacketData(SocketInputStream& in)
{
	this->field_6365_a = in.readInt();
	this->field_6364_b = in.readInt();
}

void Packet39::writePacketData(SocketOutputStream& out)
{
	out.writeInt(this->field_6365_a);
	out.writeInt(this->field_6364_b);
}

void Packet39::processPacket(NetHandler* handler)
{
	handler->func_6497_a(this);
}

int Packet39::getPacketSize()
{
	return 8;
}

int Packet39::getPacketId() const
{
	return 39;
}
