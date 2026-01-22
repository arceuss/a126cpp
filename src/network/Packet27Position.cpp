#include "network/Packet27Position.h"
#include "network/NetHandler.h"

Packet27Position::Packet27Position()
	: field_22039_a(0.0f)
	, field_22038_b(0.0f)
	, field_22041_e(0.0f)
	, field_22040_f(0.0f)
	, field_22043_c(false)
	, field_22042_d(false)
{
}

void Packet27Position::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_22039_a = var1.readFloat();
	this->field_22039_a = in.readFloat();
	
	// this.field_22038_b = var1.readFloat();
	this->field_22038_b = in.readFloat();
	
	// this.field_22041_e = var1.readFloat();
	this->field_22041_e = in.readFloat();
	
	// this.field_22040_f = var1.readFloat();
	this->field_22040_f = in.readFloat();
	
	// this.field_22043_c = var1.readBoolean();
	this->field_22043_c = in.readBoolean();
	
	// this.field_22042_d = var1.readBoolean();
	this->field_22042_d = in.readBoolean();
}

void Packet27Position::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeFloat(this.field_22039_a);
	out.writeFloat(this->field_22039_a);
	
	// var1.writeFloat(this.field_22038_b);
	out.writeFloat(this->field_22038_b);
	
	// var1.writeFloat(this.field_22041_e);
	out.writeFloat(this->field_22041_e);
	
	// var1.writeFloat(this.field_22040_f);
	out.writeFloat(this->field_22040_f);
	
	// var1.writeBoolean(this.field_22043_c);
	out.writeBoolean(this->field_22043_c);
	
	// var1.writeBoolean(this.field_22042_d);
	out.writeBoolean(this->field_22042_d);
}

void Packet27Position::processPacket(NetHandler* handler)
{
	// Java: var1.func_22185_a(this);
	handler->func_22185_a(this);
}

int Packet27Position::getPacketSize()
{
	// Java: return 18;
	// float (4) + float (4) + float (4) + float (4) + boolean (1) + boolean (1) = 18
	return 18;
}

int Packet27Position::getPacketId() const
{
	return 27;
}
