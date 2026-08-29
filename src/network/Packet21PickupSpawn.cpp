#include "network/Packet21PickupSpawn.h"
#include "network/NetHandler.h"
#include "world/entity/item/EntityItem.h"
#include <cmath>
#include <limits>

namespace
{
int_t javaDoubleToInt(double value)
{
	if (std::isnan(value))
		return 0;
	if (value >= static_cast<double>((std::numeric_limits<int_t>::max)()))
		return (std::numeric_limits<int_t>::max)();
	if (value <= static_cast<double>((std::numeric_limits<int_t>::min)()))
		return (std::numeric_limits<int_t>::min)();
	return static_cast<int_t>(value);
}

int_t javaFloor(double value)
{
	const int_t truncated = javaDoubleToInt(value);
	if (!(value < static_cast<double>(truncated)))
		return truncated;
	return static_cast<int_t>(static_cast<uint_t>(truncated) - 1u);
}

byte_t narrowByte(int_t value)
{
	const int_t low = static_cast<int_t>(static_cast<uint_t>(value) & 0xFFu);
	return static_cast<byte_t>(low >= 128 ? low - 256 : low);
}
}

Packet21PickupSpawn::Packet21PickupSpawn()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, rotation(0)
	, pitch(0)
	, roll(0)
	, itemId(0)
	, count(0)
	, itemDamage(0)
{
}

Packet21PickupSpawn::Packet21PickupSpawn(EntityItem &entityItem)
	: entityId(entityItem.entityId)
	, xPosition(javaFloor(entityItem.x * 32.0))
	, yPosition(javaFloor(entityItem.y * 32.0))
	, zPosition(javaFloor(entityItem.z * 32.0))
	, rotation(narrowByte(javaDoubleToInt(entityItem.xd * 128.0)))
	, pitch(narrowByte(javaDoubleToInt(entityItem.yd * 128.0)))
	, roll(narrowByte(javaDoubleToInt(entityItem.zd * 128.0)))
	, itemId(entityItem.item.itemID)
	, count(entityItem.item.stackSize)
	, itemDamage(entityItem.item.itemDamage)
{
}

void Packet21PickupSpawn::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.itemId = var1.readShort();
	this->itemId = in.readShort();
	
	// this.count = var1.readByte();
	this->count = in.readByte();
	
	// this.itemDamage = var1.readShort();
	this->itemDamage = in.readShort();
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.rotation = var1.readByte();
	this->rotation = in.readByte();
	
	// this.pitch = var1.readByte();
	this->pitch = in.readByte();
	
	// this.roll = var1.readByte();
	this->roll = in.readByte();
}

void Packet21PickupSpawn::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeShort(this.itemId);
	out.writeShort(static_cast<short_t>(this->itemId));
	
	// var1.writeByte(this.count);
	out.writeByte(narrowByte(this->count));
	
	// var1.writeShort(this.itemDamage);
	out.writeShort(static_cast<short_t>(this->itemDamage));
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.writeByte(this.rotation);
	out.writeByte(this->rotation);
	
	// var1.writeByte(this.pitch);
	out.writeByte(this->pitch);
	
	// var1.writeByte(this.roll);
	out.writeByte(this->roll);
}

void Packet21PickupSpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handlePickupSpawn(this);
	handler->handlePickupSpawn(this);
}

int Packet21PickupSpawn::getPacketSize()
{
	return 24;
}

int Packet21PickupSpawn::getPacketId() const
{
	return 21;
}
