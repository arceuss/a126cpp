#include "network/Packet71Weather.h"
#include "network/NetHandler.h"

Packet71Weather::Packet71Weather()
	: field_27054_a(0)
	, field_27053_b(0)
	, field_27057_c(0)
	, field_27056_d(0)
	, field_27055_e(0)
{
}

void Packet71Weather::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_27054_a = var1.readInt();
	this->field_27054_a = in.readInt();
	
	// this.field_27055_e = var1.readByte();
	this->field_27055_e = static_cast<int_t>(in.read() & 0xFF);
	
	// this.field_27053_b = var1.readInt();
	this->field_27053_b = in.readInt();
	
	// this.field_27057_c = var1.readInt();
	this->field_27057_c = in.readInt();
	
	// this.field_27056_d = var1.readInt();
	this->field_27056_d = in.readInt();
}

void Packet71Weather::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_27054_a);
	out.writeInt(this->field_27054_a);
	
	// var1.writeByte(this.field_27055_e);
	out.writeByte(static_cast<byte_t>(this->field_27055_e));
	
	// var1.writeInt(this.field_27053_b);
	out.writeInt(this->field_27053_b);
	
	// var1.writeInt(this.field_27057_c);
	out.writeInt(this->field_27057_c);
	
	// var1.writeInt(this.field_27056_d);
	out.writeInt(this->field_27056_d);
}

void Packet71Weather::processPacket(NetHandler* handler)
{
	// Java: var1.handleWeather(this);
	handler->handleWeather(this);
}

int Packet71Weather::getPacketSize()
{
	// Java: return 17;
	// int (4) + byte (1) + int (4) + int (4) + int (4) = 17
	return 17;
}

int Packet71Weather::getPacketId() const
{
	return 71;
}
