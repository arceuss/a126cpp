#include "network/Packet20NamedEntitySpawn.h"
#include "network/NetHandler.h"
#include "world/entity/player/Player.h"
#include <cmath>
#include <limits>

namespace
{
int_t javaFloatToInt(float value)
{
	if (std::isnan(value))
		return 0;
	if (value >= static_cast<float>((std::numeric_limits<int_t>::max)()))
		return (std::numeric_limits<int_t>::max)();
	if (value <= static_cast<float>((std::numeric_limits<int_t>::min)()))
		return (std::numeric_limits<int_t>::min)();
	return static_cast<int_t>(value);
}

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

Packet20NamedEntitySpawn::Packet20NamedEntitySpawn()
	: entityId(0)
	, name(u"")
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, rotation(0)
	, pitch(0)
	, currentItem(0)
{
}

Packet20NamedEntitySpawn::Packet20NamedEntitySpawn(Player &player)
	: entityId(player.entityId)
	, name(player.name)
	, xPosition(javaFloor(player.x * 32.0))
	, yPosition(javaFloor(player.y * 32.0))
	, zPosition(javaFloor(player.z * 32.0))
	, rotation(narrowByte(javaFloatToInt(player.yRot * 256.0f / 360.0f)))
	, pitch(narrowByte(javaFloatToInt(player.xRot * 256.0f / 360.0f)))
	, currentItem(0)
{
	ItemStack *item = player.inventory.getCurrentItem();
	currentItem = item == nullptr ? 0 : item->itemID;
}

void Packet20NamedEntitySpawn::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.name = readString(var1, 16);
	this->name = Packet::readString(in, 16);
	
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
	
	// this.currentItem = var1.readShort();
	this->currentItem = in.readShort();
}

void Packet20NamedEntitySpawn::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// writeString(this.name, var1);
	Packet::writeString(this->name, out);
	
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
	
	// var1.writeShort(this.currentItem);
	out.writeShort(static_cast<short_t>(this->currentItem));
}

void Packet20NamedEntitySpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handleNamedEntitySpawn(this);
	handler->handleNamedEntitySpawn(this);
}

int Packet20NamedEntitySpawn::getPacketSize()
{
	return 28;
}

int Packet20NamedEntitySpawn::getPacketId() const
{
	return 20;
}
