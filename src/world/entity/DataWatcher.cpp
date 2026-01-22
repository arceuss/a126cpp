#include "world/entity/DataWatcher.h"
#include "network/Packet.h"
#include "java/IOUtil.h"
#include <iostream>
#include "world/item/ItemStack.h"
#include "world/phys/ChunkCoordinates.h"
#include <stdexcept>
#include <typeindex>
#include <typeinfo>

// Initialize static data types map
std::unordered_map<std::type_index, int> DataWatcher::dataTypes;

DataWatcher::DataWatcher() : objectChanged(false)
{
	initializeDataTypes();
}

void DataWatcher::initializeDataTypes()
{
	static bool initialized = false;
	if (initialized) return;
	initialized = true;
	
	// Alpha 1.2.6: Static block from DataWatcher.java lines 170-178
	dataTypes[std::type_index(typeid(byte_t))] = 0;
	dataTypes[std::type_index(typeid(short_t))] = 1;
	dataTypes[std::type_index(typeid(int))] = 2;
	dataTypes[std::type_index(typeid(float))] = 3;
	dataTypes[std::type_index(typeid(jstring))] = 4;
	dataTypes[std::type_index(typeid(ItemStack))] = 5;
	dataTypes[std::type_index(typeid(ChunkCoordinates))] = 6;
}

void DataWatcher::addObject(int dataValueId, const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& object)
{
	// Alpha 1.2.6: DataWatcher.addObject() - lines 17-29
	int objectType = -1;
	
	// Determine type from variant
	std::visit([&objectType](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		auto it = dataTypes.find(std::type_index(typeid(T)));
		if (it != dataTypes.end())
		{
			objectType = it->second;
		}
	}, object);
	
	if (objectType == -1)
	{
		throw std::invalid_argument("Unknown data type");
	}
	
	if (dataValueId > 31)
	{
		throw std::invalid_argument("Data value id is too big with " + std::to_string(dataValueId) + "! (Max is 31)");
	}
	
	if (watchedObjects.find(dataValueId) != watchedObjects.end())
	{
		throw std::invalid_argument("Duplicate id value for " + std::to_string(dataValueId) + "!");
	}
	
	watchedObjects[dataValueId] = std::make_shared<WatchableObject>(objectType, dataValueId, object);
}

byte_t DataWatcher::getWatchableObjectByte(int dataValueId) const
{
	auto it = watchedObjects.find(dataValueId);
	if (it == watchedObjects.end())
	{
		throw std::runtime_error("WatchableObject not found for id " + std::to_string(dataValueId));
	}
	return it->second->getByte();
}

short_t DataWatcher::getWatchableObjectShort(int dataValueId) const
{
	auto it = watchedObjects.find(dataValueId);
	if (it == watchedObjects.end())
	{
		throw std::runtime_error("WatchableObject not found for id " + std::to_string(dataValueId));
	}
	return it->second->getShort();
}

int DataWatcher::getWatchableObjectInt(int dataValueId) const
{
	auto it = watchedObjects.find(dataValueId);
	if (it == watchedObjects.end())
	{
		throw std::runtime_error("WatchableObject not found for id " + std::to_string(dataValueId));
	}
	return it->second->getInt();
}

bool DataWatcher::hasWatchableObject(int dataValueId) const
{
	return watchedObjects.find(dataValueId) != watchedObjects.end();
}

jstring DataWatcher::getWatchableObjectString(int dataValueId) const
{
	auto it = watchedObjects.find(dataValueId);
	if (it == watchedObjects.end())
	{
		throw std::runtime_error("WatchableObject not found for id " + std::to_string(dataValueId));
	}
	return it->second->getString();
}

ItemStack DataWatcher::getWatchableObjectItemStack(int dataValueId) const
{
	auto it = watchedObjects.find(dataValueId);
	if (it == watchedObjects.end())
	{
		throw std::runtime_error("WatchableObject not found for id " + std::to_string(dataValueId));
	}
	return it->second->getItemStack();
}

void DataWatcher::updateObject(int dataValueId, const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& object)
{
	// Alpha 1.2.6: DataWatcher.updateObject() - lines 43-50
	auto it = watchedObjects.find(dataValueId);
	if (it == watchedObjects.end())
	{
		return;
	}
	
	auto& watchable = it->second;
	
	// Check if object changed (simplified - in Java it uses equals())
	bool changed = false;
	std::visit([&watchable, &changed](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, byte_t>)
		{
			changed = (watchable->getByte() != arg);
		}
		else if constexpr (std::is_same_v<T, short_t>)
		{
			changed = (watchable->getShort() != arg);
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			changed = (watchable->getInt() != arg);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			changed = (watchable->getFloat() != arg);
		}
		else if constexpr (std::is_same_v<T, jstring>)
		{
			changed = (watchable->getString() != arg);
		}
		else if constexpr (std::is_same_v<T, ItemStack>)
		{
			// ItemStack comparison - simplified
			changed = true;  // Always consider changed for now
		}
		else if constexpr (std::is_same_v<T, ChunkCoordinates>)
		{
			changed = (watchable->getChunkCoordinates().x != arg.x || 
			           watchable->getChunkCoordinates().y != arg.y || 
			           watchable->getChunkCoordinates().z != arg.z);
		}
	}, object);
	
	if (changed)
	{
		std::visit([&watchable](auto&& arg) {
			watchable->setObject(arg);
		}, object);
		watchable->setWatching(true);
		objectChanged = true;
	}
}

void DataWatcher::writeWatchableObjects(std::ostream& os)
{
	// Alpha 1.2.6: DataWatcher.writeWatchableObjects() - lines 66-75
	for (const auto& pair : watchedObjects)
	{
		writeWatchableObject(os, *pair.second);
	}
	IOUtil::writeByte(os, 127);  // Terminator
}

void DataWatcher::writeWatchableObject(std::ostream& os, const WatchableObject& obj)
{
	// Alpha 1.2.6: DataWatcher.writeWatchableObject() - lines 77-109
	int var2 = (obj.getObjectType() << 5 | obj.getDataValueId() & 31) & 255;
	IOUtil::writeByte(os, static_cast<byte_t>(var2));
	
	switch (obj.getObjectType())
	{
	case 0:  // Byte
		IOUtil::writeByte(os, obj.getByte());
		break;
	case 1:  // Short
		IOUtil::writeShort(os, obj.getShort());
		break;
	case 2:  // Int
		IOUtil::writeInt(os, obj.getInt());
		break;
	case 3:  // Float
		IOUtil::writeFloat(os, obj.getFloat());
		break;
	case 4:  // String
		// Java: Packet.writeString((String)var1.getObject(), var0);
		// Use Packet::writeString overload for std::ostream (matches Java exactly)
		Packet::writeString(obj.getString(), os);
		break;
	case 5:  // ItemStack
	{
		ItemStack stack = obj.getItemStack();
		IOUtil::writeShort(os, static_cast<short_t>(stack.itemID));  // shiftedIndex
		IOUtil::writeByte(os, static_cast<byte_t>(stack.stackSize));
		IOUtil::writeShort(os, static_cast<short_t>(stack.itemDamage));
		break;
	}
	case 6:  // ChunkCoordinates
	{
		ChunkCoordinates coords = obj.getChunkCoordinates();
		IOUtil::writeInt(os, coords.x);
		IOUtil::writeInt(os, coords.y);
		IOUtil::writeInt(os, coords.z);
		break;
	}
	}
}

std::vector<std::shared_ptr<WatchableObject>> DataWatcher::readWatchableObjects(std::istream& is)
{
	// Alpha 1.2.6: DataWatcher.readWatchableObjects() - lines 111-155
	std::vector<std::shared_ptr<WatchableObject>> result;
	
	byte_t var2 = IOUtil::readByte(is);
	while (var2 != 127)  // 0x7F is the terminator
	{
		int var3 = (static_cast<unsigned char>(var2) & 0xE0) >> 5;  // Type (0-6)
		int var4 = static_cast<unsigned char>(var2) & 0x1F;  // ID (0-31)
		
		std::shared_ptr<WatchableObject> var5 = nullptr;
		
		switch (var3)
		{
		case 0:  // Byte
			var5 = std::make_shared<WatchableObject>(var3, var4, IOUtil::readByte(is));
			break;
		case 1:  // Short
			var5 = std::make_shared<WatchableObject>(var3, var4, IOUtil::readShort(is));
			break;
		case 2:  // Int
			var5 = std::make_shared<WatchableObject>(var3, var4, IOUtil::readInt(is));
			break;
		case 3:  // Float
			var5 = std::make_shared<WatchableObject>(var3, var4, IOUtil::readFloat(is));
			break;
		case 4:  // String
			// Java: Packet.readString(var0, 64);
			// Use Packet::readString overload for std::istream (matches Java exactly)
			var5 = std::make_shared<WatchableObject>(var3, var4, Packet::readString(is, 64));
			break;
		case 5:  // ItemStack
		{
			short_t var6 = IOUtil::readShort(is);
			byte_t var7 = IOUtil::readByte(is);
			short_t var8 = IOUtil::readShort(is);
			ItemStack stack(static_cast<int_t>(var6), static_cast<int_t>(var7), static_cast<int_t>(var8));
			var5 = std::make_shared<WatchableObject>(var3, var4, stack);
			break;
		}
		case 6:  // ChunkCoordinates
		{
			int var9 = IOUtil::readInt(is);
			int var10 = IOUtil::readInt(is);
			int var11 = IOUtil::readInt(is);
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
		var2 = IOUtil::readByte(is);
	}
	
	return result;
}

void DataWatcher::writeObjectsInListToStream(const std::vector<std::shared_ptr<WatchableObject>>& objects, std::ostream& os)
{
	// Alpha 1.2.6: DataWatcher.writeObjectsInListToStream() - lines 53-64
	if (!objects.empty())
	{
		for (const auto& obj : objects)
		{
			writeWatchableObject(os, *obj);
		}
	}
	IOUtil::writeByte(os, 127);  // Terminator
}

void DataWatcher::updateWatchedObjectsFromList(const std::vector<std::shared_ptr<WatchableObject>>& objects)
{
	// Alpha 1.2.6: DataWatcher.updateWatchedObjectsFromList() - lines 157-168
	// Java: public void updateWatchedObjectsFromList(List var1) {
	//       Iterator var2 = var1.iterator();
	//       while(var2.hasNext()) {
	//           WatchableObject var3 = (WatchableObject)var2.next();
	//           WatchableObject var4 = (WatchableObject)this.watchedObjects.get(Integer.valueOf(var3.getDataValueId()));
	//           if(var4 != null) {
	//               var4.setObject(var3.getObject());
	//           }
	//       }
	// }
	for (const auto& obj : objects)
	{
		auto it = watchedObjects.find(obj->getDataValueId());
		if (it != watchedObjects.end())
		{
			// Java: var4.setObject(var3.getObject()) - directly sets the object
			// Note: Java's WatchableObject.setObject() sets both the object and marks it as changed
			// Update the object value
			switch (obj->getObjectType())
			{
			case 0:
				it->second->setObject(obj->getByte());
				break;
			case 1:
				it->second->setObject(obj->getShort());
				break;
			case 2:
				it->second->setObject(obj->getInt());
				break;
			case 3:
				it->second->setObject(obj->getFloat());
				break;
			case 4:
				it->second->setObject(obj->getString());
				break;
			case 5:
				it->second->setObject(obj->getItemStack());
				break;
			case 6:
				it->second->setObject(obj->getChunkCoordinates());
				break;
			}
			// Java: WatchableObject.setObject() sets isWatching = true and objectChanged = true
			// Mark as watching and changed (matching Java behavior)
			it->second->setWatching(true);
			objectChanged = true;
		}
		else
		{
			// Java doesn't add missing objects here, but for robustness in multiplayer,
			// we should add the object if it doesn't exist (server sent it, so it should exist)
			// This handles cases where the entity wasn't fully initialized or metadata came before spawn
			// Create a new WatchableObject with the same type and value
			std::shared_ptr<WatchableObject> newObj = nullptr;
			switch (obj->getObjectType())
			{
			case 0:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getByte());
				break;
			case 1:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getShort());
				break;
			case 2:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getInt());
				break;
			case 3:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getFloat());
				break;
			case 4:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getString());
				break;
			case 5:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getItemStack());
				break;
			case 6:
				newObj = std::make_shared<WatchableObject>(obj->getObjectType(), obj->getDataValueId(), obj->getChunkCoordinates());
				break;
			}
			if (newObj != nullptr)
			{
				watchedObjects[obj->getDataValueId()] = newObj;
				objectChanged = true;
			}
		}
	}
}
