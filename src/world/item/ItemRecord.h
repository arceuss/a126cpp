#pragma once

#include "world/item/Item.h"
#include "java/String.h"

// Beta 1.2: RecordingItem.java
// Alpha 1.2.6: ItemRecord.java
class ItemRecord : public Item
{
private:
	jstring recording;  // Beta: recording (RecordingItem.java:8), Alpha: recordName (ItemRecord.java:4)

public:
	ItemRecord(int_t id, const jstring &recording);
	
	// Beta: useOn() - plays record in jukebox (RecordingItem.java:17-27)
	virtual bool useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face) override;
	
	// Alpha 1.2.6: Public getter for record name
	const jstring& getRecordingName() const { return recording; }
};
