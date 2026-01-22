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
		
		if (var5 != nullptr)
		{
			result.push_back(var5);
		}
		
		// Read next byte
		var2 = in.readByte();
	}
	
	return result;
}

void Packet40EntityMetadata::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.field_21048_b = DataWatcher.readWatchableObjects(var1);
	this->field_21048_b = readWatchableObjectsFromSocket(in);
}

void Packet40EntityMetadata::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// DataWatcher.writeObjectsInListToStream(this.field_21048_b, var1);
	// Write metadata from field_21048_b list
	for (const auto& obj : field_21048_b)
	{
		// Write header byte: (objectType << 5) | (dataValueId & 31)
		int header = (obj->getObjectType() << 5) | (obj->getDataValueId() & 31);
		out.writeByte(static_cast<byte_t>(header & 0xFF));
		
		// Write object value based on type
		switch (obj->getObjectType())
		{
		case 0:  // Byte
			out.writeByte(obj->getByte());
			break;
		case 1:  // Short
			out.writeShort(obj->getShort());
			break;
		case 2:  // Int
			out.writeInt(obj->getInt());
			break;
		case 3:  // Float
			out.writeFloat(obj->getFloat());
			break;
		case 4:  // String
			Packet::writeString(obj->getString(), out);
			break;
		case 5:  // ItemStack
		{
			ItemStack stack = obj->getItemStack();
			out.writeShort(static_cast<short_t>(stack.itemID));
			out.writeByte(static_cast<byte_t>(stack.stackSize));
			out.writeShort(static_cast<short_t>(stack.itemDamage));
			break;
		}
		case 6:  // ChunkCoordinates
		{
			ChunkCoordinates coords = obj->getChunkCoordinates();
			out.writeInt(coords.x);
			out.writeInt(coords.y);
			out.writeInt(coords.z);
			break;
		}
		}
	}
	// Write terminator
	out.writeByte(127);  // 0x7F
}

void Packet40EntityMetadata::processPacket(NetHandler* handler)
{
	// Java: var1.handleEntityMetadata(this);
	handler->handleEntityMetadata(this);
}

int Packet40EntityMetadata::getPacketSize()
{
	// Java: variable size - 5 bytes + metadata
	// int (4) + metadata (variable, at least 1 byte for terminator 0x7F)
	// For now, return a minimum size
	return 5 + static_cast<int>(field_21048_b.size() * 2);  // Approximate
}

int Packet40EntityMetadata::getPacketId() const
{
	return 40;
}
