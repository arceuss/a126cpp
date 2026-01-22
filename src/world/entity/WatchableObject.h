#pragma once

#include <variant>
#include "java/Type.h"
#include "java/String.h"
#include "world/item/ItemStack.h"
#include "world/phys/ChunkCoordinates.h"

// Alpha 1.2.6 WatchableObject - holds metadata value with type and ID
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/WatchableObject.java
class WatchableObject
{
private:
	int objectType;  // Type: 0=Byte, 1=Short, 2=Int, 3=Float, 4=String, 5=ItemStack, 6=ChunkCoordinates
	int dataValueId;  // ID (0-31)
	std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates> watchedObject;
	bool isWatching;

public:
	WatchableObject(int objectType, int dataValueId, const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& object)
		: objectType(objectType), dataValueId(dataValueId), watchedObject(object), isWatching(true) {}
	
	int getDataValueId() const { return dataValueId; }
	int getObjectType() const { return objectType; }
	bool getIsWatching() const { return isWatching; }
	void setWatching(bool watching) { isWatching = watching; }
	
	// Get object as variant (for type-safe access)
	const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& getObject() const
	{
		return watchedObject;
	}
	
	// Set object from variant
	void setObject(const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& obj)
	{
		watchedObject = obj;
	}
	
	// Set object from specific type
	template<typename T>
	void setObject(const T& obj)
	{
		watchedObject = obj;
	}
	
	// Helper methods to get typed values
	byte_t getByte() const { return std::get<byte_t>(watchedObject); }
	short_t getShort() const { return std::get<short_t>(watchedObject); }
	int getInt() const { return std::get<int>(watchedObject); }
	float getFloat() const { return std::get<float>(watchedObject); }
	jstring getString() const { return std::get<jstring>(watchedObject); }
	ItemStack getItemStack() const { return std::get<ItemStack>(watchedObject); }
	ChunkCoordinates getChunkCoordinates() const { return std::get<ChunkCoordinates>(watchedObject); }
	
	const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& getObjectVariant() const { return watchedObject; }
};
