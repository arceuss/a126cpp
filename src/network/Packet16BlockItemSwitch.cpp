#include "network/Packet16BlockItemSwitch.h"
#include "network/NetHandler.h"

Packet16BlockItemSwitch::Packet16BlockItemSwitch()
	: id(0)
{
}

Packet16BlockItemSwitch::Packet16BlockItemSwitch(int_t slotId)
	: id(slotId)
{
}

void Packet16BlockItemSwitch::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.id = var1.readShort();
	this->id = in.readShort();
}

void Packet16BlockItemSwitch::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeShort(this.id);
	out.writeShort(static_cast<short_t>(this->id));
}

void Packet16BlockItemSwitch::processPacket(NetHandler* handler)
{
	// Java: var1.handleBlockItemSwitch(this);
	handler->handleBlockItemSwitch(this);
}

int Packet16BlockItemSwitch::getPacketSize()
{
	// Java: return 2;
	// short (2) = 2
	return 2;
}

int Packet16BlockItemSwitch::getPacketId() const
{
	return 16;
}
