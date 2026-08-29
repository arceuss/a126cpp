#include "network/Packet33RelEntityMoveLook.h"

Packet33RelEntityMoveLook::Packet33RelEntityMoveLook()
{
	this->rotating = true;
}

void Packet33RelEntityMoveLook::readPacketData(SocketInputStream& in)
{
	Packet30Entity::readPacketData(in);
	this->xPosition = in.readByte();
	this->yPosition = in.readByte();
	this->zPosition = in.readByte();
	this->yaw = in.readByte();
	this->pitch = in.readByte();
}

void Packet33RelEntityMoveLook::writePacketData(SocketOutputStream& out)
{
	Packet30Entity::writePacketData(out);
	out.writeByte(this->xPosition);
	out.writeByte(this->yPosition);
	out.writeByte(this->zPosition);
	out.writeByte(this->yaw);
	out.writeByte(this->pitch);
}

int Packet33RelEntityMoveLook::getPacketSize()
{
	return 9;
}

int Packet33RelEntityMoveLook::getPacketId() const
{
	return 33;
}
