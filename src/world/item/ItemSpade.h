#pragma once

#include "world/item/ItemTool.h"

class Tile;

// Alpha 1.2.6 ItemSpade
// Reference: apclient/minecraft/src/net/minecraft/src/ItemSpade.java
class ItemSpade : public ItemTool
{
public:
	ItemSpade(int_t id, int_t tier);
	virtual ~ItemSpade() {}
	
	bool canHarvestBlock(Tile &tile) override;
};
