#pragma once

#include "world/item/ItemTool.h"

class Tile;

// Alpha 1.2.6 ItemPickaxe
// Reference: apclient/minecraft/src/net/minecraft/src/ItemPickaxe.java
class ItemPickaxe : public ItemTool
{
private:
	int_t harvestLevel;  // field_328_aY - tool tier for harvest checks

public:
	ItemPickaxe(int_t id, int_t tier);
	virtual ~ItemPickaxe() {}
	
	bool canHarvestBlock(Tile &tile) override;
};
