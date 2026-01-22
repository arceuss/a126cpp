#include "network/Packet23VehicleSpawn.h"
#include "network/NetHandler.h"

Packet23VehicleSpawn::Packet23VehicleSpawn()
	: entityId(0)
	, type(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, field_28044_i(0)
	, field_28047_e(0)
	, field_28046_f(0)
	, field_28045_g(0)
{
}

void Packet23VehicleSpawn::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.type = var1.readByte();
	this->type = static_cast<int_t>(in.read() & 0xFF);
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.field_28044_i = var1.readInt();
	this->field_28044_i = in.readInt();
	
	// if(this.field_28044_i > 0) {
	//     this.field_28047_e = var1.readShort();
	//     this.field_28046_f = var1.readShort();
	//     this.field_28045_g = var1.readShort();
	// }
	if (this->field_28044_i > 0)
	{
		this->field_28047_e = in.readShort();
		this->field_28046_f = in.readShort();
		this->field_28045_g = in.readShort();
	}
}

void Packet23VehicleSpawn::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeByte(this.type);
	out.writeByte(static_cast<byte_t>(this->type));
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.writeInt(this.field_28044_i);
	out.writeInt(this->field_28044_i);
	
	// if(this.field_28044_i > 0) {
	//     var1.writeShort(this.field_28047_e);
	//     var1.writeShort(this.field_28046_f);
	//     var1.writeShort(this.field_28045_g);
	// }
	if (this->field_28044_i > 0)
	{
		out.writeShort(static_cast<short_t>(this->field_28047_e));
		out.writeShort(static_cast<short_t>(this->field_28046_f));
		out.writeShort(static_cast<short_t>(this->field_28045_g));
	}
}

void Packet23VehicleSpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handleVehicleSpawn(this);
	handler->handleVehicleSpawn(this);
}

int Packet23VehicleSpawn::getPacketSize()
{
	// Java: variable size - 22 or 28 bytes
	// int (4) + byte (1) + int (4) + int (4) + int (4) + int (4) = 21 base
	// + 3 * short (6) if field_28044_i > 0 = 27 total
	// Actually, let's say 22 base + 6 conditional = 28 max
	return (this->field_28044_i > 0) ? 28 : 22;
}

int Packet23VehicleSpawn::getPacketId() const
{
	return 23;
}
