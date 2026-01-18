#pragma once

#include "java/Type.h"
#include <memory>
#include <vector>
#include <limits>
#include <climits>

// newb12: IntHashMap - hash map using int as keys
// Reference: newb12/util/IntHashMap.java
template<typename V>
class IntHashMap
{
private:
	static constexpr int DEFAULT_INITIAL_CAPACITY = 16;
	static constexpr int MAXIMUM_CAPACITY = 1073741824;
	static constexpr float DEFAULT_LOAD_FACTOR = 0.75F;
	
	struct Entry
	{
		int key;
		V value;
		std::unique_ptr<Entry> next;
		int hash;
		
		Entry(int hash, int key, V value, std::unique_ptr<Entry> next)
			: key(key), value(value), next(std::move(next)), hash(hash)
		{
		}
	};
	
	std::vector<std::unique_ptr<Entry>> table;
	int size = 0;
	int threshold;
	const float loadFactor = DEFAULT_LOAD_FACTOR;
	
	static int hash(int key)
	{
		key ^= (key >> 20) ^ (key >> 12);
		return key ^ (key >> 7) ^ (key >> 4);
	}
	
	static int indexFor(int hash, int length)
	{
		return hash & (length - 1);
	}
	
	void resize(int newCapacity)
	{
		if (table.size() == MAXIMUM_CAPACITY)
		{
			threshold = INT_MAX;
			return;
		}
		
		std::vector<std::unique_ptr<Entry>> newTable(newCapacity);
		
		for (size_t i = 0; i < table.size(); i++)
		{
			std::unique_ptr<Entry> entry = std::move(table[i]);
			while (entry != nullptr)
			{
				std::unique_ptr<Entry> next = std::move(entry->next);
				int newIndex = indexFor(entry->hash, newCapacity);
				entry->next = std::move(newTable[newIndex]);
				newTable[newIndex] = std::move(entry);
				entry = std::move(next);
			}
		}
		
		table = std::move(newTable);
		threshold = (int)(newCapacity * loadFactor);
	}
	
	void addEntry(int hash, int key, V value, int bucketIndex)
	{
		std::unique_ptr<Entry> next = std::move(table[bucketIndex]);
		table[bucketIndex] = std::make_unique<Entry>(hash, key, value, std::move(next));
		if (size++ >= threshold)
		{
			resize(2 * table.size());
		}
	}
	
	Entry* getEntry(int key) const
	{
		int hashValue = hash(key);
		int bucketIndex = indexFor(hashValue, table.size());
		
		Entry* entry = table[bucketIndex].get();
		while (entry != nullptr)
		{
			if (entry->key == key)
			{
				return entry;
			}
			entry = entry->next.get();
		}
		
		return nullptr;
	}

public:
	IntHashMap()
		: threshold(12)
	{
		table.resize(DEFAULT_INITIAL_CAPACITY);
	}
	
	int getSize() const { return size; }
	bool isEmpty() const { return size == 0; }
	
	V get(int key) const
	{
		Entry* entry = getEntry(key);
		return entry != nullptr ? entry->value : V();
	}
	
	bool containsKey(int key) const
	{
		return getEntry(key) != nullptr;
	}
	
	void put(int key, V value)
	{
		int hashValue = hash(key);
		int bucketIndex = indexFor(hashValue, table.size());
		
		Entry* existing = table[bucketIndex].get();
		while (existing != nullptr)
		{
			if (existing->key == key)
			{
				existing->value = value;
				return;
			}
			existing = existing->next.get();
		}
		
		addEntry(hashValue, key, value, bucketIndex);
	}
	
	V remove(int key)
	{
		int hashValue = hash(key);
		int bucketIndex = indexFor(hashValue, table.size());
		
		Entry* prev = nullptr;
		Entry* entry = table[bucketIndex].get();
		
		while (entry != nullptr)
		{
			if (entry->key == key)
			{
				V value = entry->value;
				if (prev == nullptr)
				{
					table[bucketIndex] = std::move(entry->next);
				}
				else
				{
					prev->next = std::move(entry->next);
				}
				size--;
				return value;
			}
			prev = entry;
			entry = entry->next.get();
		}
		
		return V();
	}
	
	void clear()
	{
		for (size_t i = 0; i < table.size(); i++)
		{
			table[i].reset();
		}
		size = 0;
	}
};