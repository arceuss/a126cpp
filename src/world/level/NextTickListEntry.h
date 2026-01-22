#pragma once

#include <cstddef>
#include "java/Type.h"

// Alpha 1.2.6 NextTickListEntry
// Reference: apclient/minecraft/src/net/minecraft/src/NextTickListEntry.java
struct NextTickListEntry
{
	int_t xCoord;
	int_t yCoord;
	int_t zCoord;
	int_t blockID;
	long_t scheduledTime;
	long_t tickEntryID;  // Unique ID for deterministic ordering when scheduledTime ties

	NextTickListEntry(int_t x, int_t y, int_t z, int_t blockId);
	
	// Alpha: NextTickListEntry.equals() compares x, y, z, blockID (NextTickListEntry.java:19-26)
	bool operator==(const NextTickListEntry &other) const;
	
	// Alpha: NextTickListEntry.hashCode() uses formula for HashSet (NextTickListEntry.java:28-30)
	size_t hash() const;
	
	// Alpha: NextTickListEntry.compareTo() compares scheduledTime, then tickEntryID (NextTickListEntry.java:37-38)
	bool operator<(const NextTickListEntry &other) const;
};

// Hash function for unordered_set
namespace std {
	template<>
	struct hash<NextTickListEntry> {
		size_t operator()(const NextTickListEntry &entry) const {
			return entry.hash();
		}
	};
}
