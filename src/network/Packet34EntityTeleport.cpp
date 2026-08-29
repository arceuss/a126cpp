#include "network/Packet34EntityTeleport.h"
#include "network/NetHandler.h"
#include "util/Mth.h"
#include "world/entity/Entity.h"

#include <cmath>
#include <limits>

namespace
{
byte_t javaIntToByte(int_t value)
{
	const ubyte_t low = static_cast<ubyte_t>(static_cast<uint_t>(value));
	return low <= 127 ? static_cast<byte_t>(low) : static_cast<byte_t>(static_cast<int_t>(low) - 256);
}

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

byte_t javaFloatToByte(float value)
{
	return javaIntToByte(javaFloatToInt(value));
}
}

Packet34EntityTeleport::Packet34EntityTeleport()
	: entityId(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, yaw(0)
	, pitch(0)
{
}

Packet34EntityTeleport::Packet34EntityTeleport(const Entity& entity)
	: entityId(entity.entityId)
	, xPosition(Mth::floor(entity.x * 32.0))
	, yPosition(Mth::floor(entity.y * 32.0))
	, zPosition(Mth::floor(entity.z * 32.0))
	, yaw(javaFloatToByte(entity.yRot * 256.0f / 360.0f))
	, pitch(javaFloatToByte(entity.xRot * 256.0f / 360.0f))
{
}

void Packet34EntityTeleport::readPacketData(SocketInputStream& in)
{
	this->entityId = in.readInt();
	this->xPosition = in.readInt();
	this->yPosition = in.readInt();
	this->zPosition = in.readInt();
	this->yaw = javaIntToByte(in.read());
	this->pitch = javaIntToByte(in.read());
}

void Packet34EntityTeleport::writePacketData(SocketOutputStream& out)
{
	out.writeInt(this->entityId);
	out.writeInt(this->xPosition);
	out.writeInt(this->yPosition);
	out.writeInt(this->zPosition);
	out.write(this->yaw);
	out.write(this->pitch);
}

void Packet34EntityTeleport::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntityTeleport(this);
	handler->handleEntityTeleport(this);
}

int Packet34EntityTeleport::getPacketSize()
{
	return 34;
}

int Packet34EntityTeleport::getPacketId() const
{
	return 34;
}
