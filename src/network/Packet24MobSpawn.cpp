#include "network/Packet24MobSpawn.h"
#include "network/NetHandler.h"
#include "world/item/ItemStack.h"
#include "world/phys/ChunkCoordinates.h"

Packet24MobSpawn::Packet24MobSpawn()
	: entityId(0)
	, type(0)
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, yaw(0)
	, pitch(0)
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
		
		if (var5 != nullptr)
		{
			result.push_back(var5);
		}
		
		// Read next byte
		var2 = in.readByte();
	}
	
	return result;
}

void Packet24MobSpawn::readPacketData(SocketInputStream& in)
{
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
	
	// this.metaData.writeWatchableObjects(var1);
	// Write metadata from receivedMetadata list (matches DataWatcher.writeObjectsInListToStream logic)
	for (const auto& obj : receivedMetadata)
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

void Packet24MobSpawn::processPacket(NetHandler* handler)
{
	// Java: var1.handleMobSpawn(this);
	handler->handleMobSpawn(this);
}

int Packet24MobSpawn::getPacketSize()
{
	// Java: variable size - 20 bytes + metadata
	// int (4) + byte (1) + int (4) + int (4) + int (4) + byte (1) + byte (1) = 19 base
	// + metadata (variable, at least 1 byte for terminator 0x7F)
	// Actually, let's say 20 base + variable metadata
	// For now, return a minimum size
	return 20 + static_cast<int>(receivedMetadata.size() * 2);  // Approximate
}

int Packet24MobSpawn::getPacketId() const
{
	return 24;
}
