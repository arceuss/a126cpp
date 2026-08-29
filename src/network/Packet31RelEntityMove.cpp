#include "network/Packet31RelEntityMove.h"

Packet31RelEntityMove::Packet31RelEntityMove() = default;

void Packet31RelEntityMove::readPacketData(SocketInputStream& in)
{
	Packet30Entity::readPacketData(in);
	this->xPosition = in.readByte();
	this->yPosition = in.readByte();
	this->zPosition = in.readByte();
}

void Packet31RelEntityMove::writePacketData(SocketOutputStream& out)
{
	Packet30Entity::writePacketData(out);
	out.writeByte(this->xPosition);
	out.writeByte(this->yPosition);
	out.writeByte(this->zPosition);
}

int Packet31RelEntityMove::getPacketSize()
{
	return 7;
}

int Packet31RelEntityMove::getPacketId() const
{
	return 31;
}
