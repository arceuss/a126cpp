#include "world/level/NextTickListEntry.h"

// Static counter for unique tick entry IDs
static long_t nextTickEntryID = 0L;

// Alpha: NextTickListEntry.java:12-17
NextTickListEntry::NextTickListEntry(int_t x, int_t y, int_t z, int_t blockId)
	: xCoord(x), yCoord(y), zCoord(z), blockID(blockId), scheduledTime(0), tickEntryID(nextTickEntryID++)
{
}

// Alpha: NextTickListEntry.equals() (NextTickListEntry.java:19-26)
bool NextTickListEntry::operator==(const NextTickListEntry &other) const
{
	return xCoord == other.xCoord && yCoord == other.yCoord && 
	       zCoord == other.zCoord && blockID == other.blockID;
}

// Alpha: NextTickListEntry.hashCode() (NextTickListEntry.java:28-30)
size_t NextTickListEntry::hash() const
{
	return ((xCoord * 128 * 1024 + zCoord * 128 + yCoord) * 256 + blockID);
}

// Alpha: NextTickListEntry.compareTo() compares scheduledTime, then tickEntryID (NextTickListEntry.java:37-38)
bool NextTickListEntry::operator<(const NextTickListEntry &other) const
{
	if (scheduledTime < other.scheduledTime)
		return true;
	if (scheduledTime > other.scheduledTime)
		return false;
	// Tie-breaker: use tickEntryID for deterministic ordering
	return tickEntryID < other.tickEntryID;
}
