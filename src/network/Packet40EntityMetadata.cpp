#include "network/Packet40EntityMetadata.h"
#include "network/NetHandler.h"
#include "world/item/ItemStack.h"
#include "world/phys/ChunkCoordinates.h"

Packet40EntityMetadata::Packet40EntityMetadata()
	: entityId(0)
{
}

std::vector<std::shared_ptr<WatchableObject>> Packet40EntityMetadata::readWatchableObjectsFromSocket(SocketInputStream& in)
{
	std::vector<std::shared_ptr<WatchableObject>> result;
	byte_t header = in.readByte();
	while (header != 127)
	{
		const int objectType = (static_cast<ubyte_t>(header) & 0xE0) >> 5;
		const int dataValueId = static_cast<ubyte_t>(header) & 0x1F;
		std::shared_ptr<WatchableObject> object;
		switch (objectType)
		{
		case 0:
			object = std::make_shared<WatchableObject>(objectType, dataValueId, in.readByte());
			break;
		case 1:
			object = std::make_shared<WatchableObject>(objectType, dataValueId, in.readShort());
			break;
		case 2:
			object = std::make_shared<WatchableObject>(objectType, dataValueId, in.readInt());
			break;
		case 3:
			object = std::make_shared<WatchableObject>(objectType, dataValueId, in.readFloat());
			break;
		case 4:
			object = std::make_shared<WatchableObject>(objectType, dataValueId, Packet::readString(in, 64));
			break;
		case 5:
		{
			const short_t shiftedIndex = in.readShort();
			const byte_t stackSize = in.readByte();
			const short_t itemDamage = in.readShort();
			object = std::make_shared<WatchableObject>(objectType, dataValueId,
				ItemStack(shiftedIndex, stackSize, itemDamage));
			break;
		}
		case 6:
		{
			const int_t x = in.readInt();
			const int_t y = in.readInt();
			const int_t z = in.readInt();
			object = std::make_shared<WatchableObject>(objectType, dataValueId,
				ChunkCoordinates(x, y, z));
			break;
		}
		}
		result.push_back(object);
		header = in.readByte();
	}
	return result;
}

void Packet40EntityMetadata::readPacketData(SocketInputStream& in)
{
	this->entityId = in.readInt();
	this->field_21048_b = readWatchableObjectsFromSocket(in);
}

void Packet40EntityMetadata::writePacketData(SocketOutputStream& out)
{
	out.writeInt(this->entityId);
	for (const auto& object : this->field_21048_b)
	{
		const int header = (object->getObjectType() << 5 | object->getDataValueId() & 31) & 255;
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

void Packet40EntityMetadata::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntityMetadata(this);
	handler->handleEntityMetadata(this);
}

int Packet40EntityMetadata::getPacketSize()
{
	return 5;
}

int Packet40EntityMetadata::getPacketId() const
{
	return 40;
}

std::vector<std::shared_ptr<WatchableObject>>& Packet40EntityMetadata::func_21047_b()
{
	return this->field_21048_b;
}
