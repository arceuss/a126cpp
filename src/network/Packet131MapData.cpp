#include "network/Packet131MapData.h"
#include "network/NetHandler.h"

Packet131MapData::Packet131MapData()
	: field_28055_a(0)
	, field_28054_b(0)
{
}

void Packet131MapData::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_28055_a = var1.readShort();
	this->field_28055_a = in.readShort();
	
	// this.field_28054_b = var1.readShort();
	this->field_28054_b = in.readShort();
	
	// this.field_28056_c = new byte[var1.readByte() & 255];
	byte_t length = in.readByte();
	this->field_28056_c.resize(static_cast<size_t>(length) & 0xFF);
	
	// var1.readFully(this.field_28056_c);
	if (!field_28056_c.empty())
	{
		in.readFully(field_28056_c.data(), field_28056_c.size());
	}
}

void Packet131MapData::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeShort(this.field_28055_a);
	out.writeShort(this->field_28055_a);
	
	// var1.writeShort(this.field_28054_b);
	out.writeShort(this->field_28054_b);
	
	// var1.writeByte(this.field_28056_c.length);
	out.writeByte(static_cast<byte_t>(field_28056_c.size()));
	
	// var1.write(this.field_28056_c);
	if (!field_28056_c.empty())
	{
		out.write(field_28056_c.data(), field_28056_c.size());
	}
}

void Packet131MapData::processPacket(NetHandler* handler)
{
	// Java: var1.handleMap(this);
	handler->handleMap(this);
}

int Packet131MapData::getPacketSize()
{
	// Java: variable size
	// short (2) + short (2) + byte (1) + byte array length
	return 5 + static_cast<int>(field_28056_c.size());
}

int Packet131MapData::getPacketId() const
{
	return 131;
}
