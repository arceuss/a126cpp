#include "network/Packet17Sleep.h"
#include "network/NetHandler.h"

Packet17Sleep::Packet17Sleep()
	: field_22045_a(0)
	, field_22044_b(0)
	, field_22048_c(0)
	, field_22047_d(0)
	, field_22046_e(0)
{
}

Packet17Sleep::Packet17Sleep(int_t entityId, int_t x, byte_t y, int_t z, byte_t inBed)
	: field_22045_a(entityId)
	, field_22044_b(x)
	, field_22048_c(static_cast<int_t>(y))
	, field_22047_d(z)
	, field_22046_e(static_cast<int_t>(inBed))
{
}

void Packet17Sleep::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_22045_a = var1.readInt();
	this->field_22045_a = in.readInt();
	
	// this.field_22046_e = var1.readByte();
	this->field_22046_e = static_cast<int_t>(in.read() & 0xFF);
	
	// this.field_22044_b = var1.readInt();
	this->field_22044_b = in.readInt();
	
	// this.field_22048_c = var1.readByte();
	this->field_22048_c = static_cast<int_t>(in.read() & 0xFF);
	
	// this.field_22047_d = var1.readInt();
	this->field_22047_d = in.readInt();
}

void Packet17Sleep::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.field_22045_a);
	out.writeInt(this->field_22045_a);
	
	// var1.writeByte(this.field_22046_e);
	out.writeByte(static_cast<byte_t>(this->field_22046_e));
	
	// var1.writeInt(this.field_22044_b);
	out.writeInt(this->field_22044_b);
	
	// var1.writeByte(this.field_22048_c);
	out.writeByte(static_cast<byte_t>(this->field_22048_c));
	
	// var1.writeInt(this.field_22047_d);
	out.writeInt(this->field_22047_d);
}

void Packet17Sleep::processPacket(NetHandler* handler)
{
	// Java: var1.handleAddToInventory(this);  // Note: Packet17Sleep uses handleAddToInventory in Java
	handler->handleAddToInventory(this);
}

int Packet17Sleep::getPacketSize()
{
	// Java: return 14;
	// int (4) + byte (1) + int (4) + byte (1) + int (4) = 14
	return 14;
}

int Packet17Sleep::getPacketId() const
{
	return 17;
}
