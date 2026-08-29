#include "network/Packet32EntityLook.h"

Packet32EntityLook::Packet32EntityLook()
{
	this->rotating = true;
}

void Packet32EntityLook::readPacketData(SocketInputStream& in)
{
	Packet30Entity::readPacketData(in);
	this->yaw = in.readByte();
	this->pitch = in.readByte();
}

void Packet32EntityLook::writePacketData(SocketOutputStream& out)
{
	Packet30Entity::writePacketData(out);
	out.writeByte(this->yaw);
	out.writeByte(this->pitch);
}

int Packet32EntityLook::getPacketSize()
{
	return 6;
}

int Packet32EntityLook::getPacketId() const
{
	return 32;
}
