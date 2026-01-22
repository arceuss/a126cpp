#pragma once

#include "world/item/Item.h"

// Beta 1.2: BucketItem.java
// Alpha 1.2.6: ItemBucket.java
class ItemBucket : public Item
{
private:
	int_t content;  // Beta: content (BucketItem.java:13), Alpha: isFull (ItemBucket.java:4)
	                // 0 = empty, >0 = block ID, <0 = special (milk = -1)

public:
	ItemBucket(int_t id, int_t content);
	
	// Beta: use() - picks up or places liquid (BucketItem.java:23-110)
	virtual ItemStack use(ItemStack &stack, Level &level, Player &player) override;
};
