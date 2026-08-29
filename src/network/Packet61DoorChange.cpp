#include "network/Packet61DoorChange.h"
#include "network/NetHandler.h"

Packet61DoorChange::Packet61DoorChange()
	: field_28050_a(0)
	, field_28049_b(0)
	, field_28053_c(0)
	, field_28052_d(0)
	, field_28051_e(0)
{
}

void Packet61DoorChange::readPacketData(SocketInputStream& in)
{
	field_28050_a = in.readInt();
	field_28053_c = in.readInt();
	field_28052_d = in.readByte();
	field_28051_e = in.readInt();
	field_28049_b = in.readInt();
}

void Packet61DoorChange::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_28050_a);
	out.writeInt(this->field_28050_a);
	
	// var1.writeInt(this.field_28053_c);
	out.writeInt(this->field_28053_c);
	
	// var1.writeByte(this.field_28052_d);
	out.writeByte(static_cast<byte_t>(this->field_28052_d));
	
	// var1.writeInt(this.field_28051_e);
	out.writeInt(this->field_28051_e);
	
	// var1.writeInt(this.field_28049_b);
	out.writeInt(this->field_28049_b);
}

void Packet61DoorChange::processPacket(NetHandler* handler)
{
	// Java: var1.func_28115_a(this);
	handler->func_28115_a(this);
}

int Packet61DoorChange::getPacketSize()
{
	return 20;
}

int Packet61DoorChange::getPacketId() const
{
	return 61;
}
