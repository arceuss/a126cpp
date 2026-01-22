#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <istream>
#include <ostream>
#include <typeindex>
#include "world/entity/WatchableObject.h"

// Forward declarations
class ItemStack;
class Packet;

// Alpha 1.2.6 DataWatcher - manages entity metadata
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/DataWatcher.java
class DataWatcher
{
private:
	static std::unordered_map<std::type_index, int> dataTypes;
	std::unordered_map<int, std::shared_ptr<WatchableObject>> watchedObjects;
	bool objectChanged;

public:
	DataWatcher();
	
	// Add/update watched objects
	void addObject(int dataValueId, const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& object);
	
	// Get watched objects by type
	byte_t getWatchableObjectByte(int dataValueId) const;
	short_t getWatchableObjectShort(int dataValueId) const;
	int getWatchableObjectInt(int dataValueId) const;
	jstring getWatchableObjectString(int dataValueId) const;
	ItemStack getWatchableObjectItemStack(int dataValueId) const;
	
	// Check if a watched object exists
	bool hasWatchableObject(int dataValueId) const;
	
	// Update object
	void updateObject(int dataValueId, const std::variant<byte_t, short_t, int, float, jstring, ItemStack, ChunkCoordinates>& object);
	
	// Write watched objects to stream
	void writeWatchableObjects(std::ostream& os);
	
	// Static methods for reading/writing from packets
	static std::vector<std::shared_ptr<WatchableObject>> readWatchableObjects(std::istream& is);
	static void writeObjectsInListToStream(const std::vector<std::shared_ptr<WatchableObject>>& objects, std::ostream& os);
	
	// Update watched objects from a list (used when receiving Packet40EntityMetadata)
	void updateWatchedObjectsFromList(const std::vector<std::shared_ptr<WatchableObject>>& objects);
	
	bool hasObjectChanged() const { return objectChanged; }
	void setObjectChanged(bool changed) { objectChanged = changed; }
	
private:
	static void writeWatchableObject(std::ostream& os, const WatchableObject& obj);
	static void initializeDataTypes();
};
