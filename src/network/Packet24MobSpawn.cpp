#include "network/Packet24MobSpawn.h"
#include "network/NetHandler.h"
#include "world/entity/DataWatcher.h"
#include "world/entity/EntityIO.h"
#include "world/entity/Mob.h"
#include "world/item/ItemStack.h"
#include "world/phys/ChunkCoordinates.h"
#include <cmath>
#include <limits>
#include <sstream>

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

Packet24MobSpawn::Packet24MobSpawn()
	: entityId(0)
	, type(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, yaw(0)
	, pitch(0)
	, metaData(nullptr)
{
}

Packet24MobSpawn::Packet24MobSpawn(Mob &mob)
	: entityId(mob.entityId)
	, type(narrowByte(EntityIO::getEncodeNumericId(mob)))
	, xPosition(javaFloor(mob.x * 32.0))
	, yPosition(javaFloor(mob.y * 32.0))
	, zPosition(javaFloor(mob.z * 32.0))
	, yaw(narrowByte(javaFloatToInt(mob.yRot * 256.0f / 360.0f)))
	, pitch(narrowByte(javaFloatToInt(mob.xRot * 256.0f / 360.0f)))
	, metaData(&mob.getDataWatcher())
{
}

std::vector<std::shared_ptr<WatchableObject>> Packet24MobSpawn::readWatchableObjectsFromSocket(SocketInputStream& in)
{
	// Matches DataWatcher.readWatchableObjects logic but for SocketInputStream
	// Java: DataWatcher.readWatchableObjects(var1)
	std::vector<std::shared_ptr<WatchableObject>> result;
	
	byte_t var2 = in.readByte();
	while (var2 != 127)  // 0x7F is the terminator
	{
		int var3 = (static_cast<unsigned char>(var2) & 0xE0) >> 5;  // Type (0-6)
		int var4 = static_cast<unsigned char>(var2) & 0x1F;  // ID (0-31)
		
		std::shared_ptr<WatchableObject> var5 = nullptr;
		
		switch (var3)
		{
		case 0:  // Byte
			var5 = std::make_shared<WatchableObject>(var3, var4, in.readByte());
			break;
		case 1:  // Short
			var5 = std::make_shared<WatchableObject>(var3, var4, in.readShort());
			break;
		case 2:  // Int
			var5 = std::make_shared<WatchableObject>(var3, var4, in.readInt());
			break;
		case 3:  // Float
			var5 = std::make_shared<WatchableObject>(var3, var4, in.readFloat());
			break;
		case 4:  // String
			// Java: Packet.readString(var1, 64)
			var5 = std::make_shared<WatchableObject>(var3, var4, Packet::readString(in, 64));
			break;
		case 5:  // ItemStack
		{
			short_t var6 = in.readShort();
			byte_t var7 = in.readByte();
			short_t var8 = in.readShort();
			ItemStack stack(static_cast<int_t>(var6), static_cast<int_t>(var7), static_cast<int_t>(var8));
			var5 = std::make_shared<WatchableObject>(var3, var4, stack);
			break;
		}
		case 6:  // ChunkCoordinates
		{
			int_t var9 = in.readInt();
			int_t var10 = in.readInt();
			int_t var11 = in.readInt();
			ChunkCoordinates coords(var9, var10, var11);
			var5 = std::make_shared<WatchableObject>(var3, var4, coords);
			break;
		}
		}
		
		result.push_back(var5);
		
		// Read next byte
		var2 = in.readByte();
	}
	
	return result;
}

void Packet24MobSpawn::readPacketData(SocketInputStream& in)
{
	this->metaData = nullptr;
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.type = var1.readByte();
	this->type = in.readByte();
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.yaw = var1.readByte();
	this->yaw = in.readByte();
	
	// this.pitch = var1.readByte();
	this->pitch = in.readByte();
	
	// this.receivedMetadata = DataWatcher.readWatchableObjects(var1);
	this->receivedMetadata = readWatchableObjectsFromSocket(in);
}

void Packet24MobSpawn::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeByte(this.type);
	out.writeByte(this->type);
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.writeByte(this.yaw);
	out.writeByte(this->yaw);
	
	// var1.writeByte(this.pitch);
	out.writeByte(this->pitch);
	
	if (this->metaData != nullptr)
	{
		std::ostringstream stream(std::ios::binary);
		this->metaData->writeWatchableObjects(stream);
		const std::string bytes = stream.str();
		out.write(reinterpret_cast<const byte_t *>(bytes.data()), bytes.size());
		return;
	}

	for (const auto &object : this->receivedMetadata)
	{
		const int_t header = (object->getObjectType() << 5 | object->getDataValueId() & 31) & 255;
		out.writeByte(static_cast<byte_t>(header));
		switch (object->getObjectType())
		{
		case 0:
			out.writeByte(object->getByte());
			break;
		case 1:
			out.writeShort(object->getShort());
			break;
		case 2:
			out.writeInt(object->getInt());
			break;
		case 3:
			out.writeFloat(object->getFloat());
			break;
		case 4:
			Packet::writeString(object->getString(), out);
			break;
		case 5:
		{
			const ItemStack stack = object->getItemStack();
			out.writeShort(static_cast<short_t>(stack.itemID));
			out.writeByte(static_cast<byte_t>(stack.stackSize));
			out.writeShort(static_cast<short_t>(stack.itemDamage));
			break;
		}
		case 6:
		{
			const ChunkCoordinates coordinates = object->getChunkCoordinates();
			out.writeInt(coordinates.x);
			out.writeInt(coordinates.y);
			out.writeInt(coordinates.z);
			break;
		}
		}
	}
	out.writeByte(127);
}

void Packet24MobSpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handleMobSpawn(this);
	handler->handleMobSpawn(this);
}

int Packet24MobSpawn::getPacketSize()
{
	return 20;
}

int Packet24MobSpawn::getPacketId() const
{
	return 24;
}

const std::vector<std::shared_ptr<WatchableObject>> &Packet24MobSpawn::getMetadata() const
{
	return this->receivedMetadata;
}
