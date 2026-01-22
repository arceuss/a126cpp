#include "world/item/ItemRecord.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/RecordPlayerTile.h"
#include "world/item/Items.h"
#include "Facing.h"

// Beta: RecordingItem(int var1, String var2) (RecordingItem.java:10-14)
// Alpha: ItemRecord(int var1, String var2) (ItemRecord.java:6-10)
ItemRecord::ItemRecord(int_t id, const jstring &recording) : Item(id), recording(recording)
{
	// Beta: this.maxStackSize = 1 (RecordingItem.java:13)
	setMaxStackSize(1);
}

// Beta: useOn() - plays record in jukebox (RecordingItem.java:17-27)
// Alpha: onItemUse() - same logic (ItemRecord.java:12-25)
bool ItemRecord::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Check if jukebox and empty (RecordingItem.java:18)
	if (level.getTile(x, y, z) == Tile::recordPlayer.id && level.getData(x, y, z) == 0)
	{
		// Beta: Set record data (RecordingItem.java:19)
		// Beta: this.id - Item.record_01.id + 1
		// newb12: var3.setData(var4, var5, var6, this.id - Item.record_01.id + 1);
		// Note: record13 should be the first record (equivalent to record_01 in Beta)
		int_t recordNumber = 0;
		if (Items::record13 != nullptr)
		{
			recordNumber = (itemID - Items::record13->getItemID()) + 1;
		}
		else
		{
			// Fallback if record13 is not initialized (shouldn't happen)
			recordNumber = 1;
		}
		level.setData(x, y, z, recordNumber);
		
		// Beta: Play music (RecordingItem.java:20)
		// newb12: var3.playStreamingMusic(this.recording, var4, var5, var6);
		level.playStreamingMusic(recording, x, y, z);
		
		// Beta: Decrement stack (RecordingItem.java:21)
		stack.stackSize--;
		return true;
	}
	
	return false;
}
