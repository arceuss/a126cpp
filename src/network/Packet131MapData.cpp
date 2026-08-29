#include "network/Packet131MapData.h"
#include "network/NetHandler.h"

Packet131MapData::Packet131MapData()
	: field_28055_a(0)
	, field_28054_b(0)
{
	isChunkDataPacket = true;
}

void Packet131MapData::readPacketData(SocketInputStream& in)
{
	this->field_28055_a = in.readShort();
	this->field_28054_b = in.readShort();
	const ubyte_t length = static_cast<ubyte_t>(in.readByte());
	this->field_28056_c.resize(length);
	if (!field_28056_c.empty())
		in.readFully(field_28056_c.data(), field_28056_c.size());
}

void Packet131MapData::writePacketData(SocketOutputStream& out)
{
	out.writeShort(this->field_28055_a);
	out.writeShort(this->field_28054_b);
	out.writeByte(static_cast<byte_t>(field_28056_c.size() & 0xFF));
	if (!field_28056_c.empty())
		out.write(field_28056_c.data(), field_28056_c.size());
}

void Packet131MapData::processPacket(NetHandler* handler)
{
	// Alpha Packet131MapData.java:39-40.
	handler->handleMap(this);
}

int Packet131MapData::getPacketSize()
{
	return 4 + static_cast<int_t>(field_28056_c.size());
}

int Packet131MapData::getPacketId() const
{
	return 131;
}
