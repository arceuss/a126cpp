#pragma once

#include "world/item/ItemTool.h"

// Alpha 1.2.6 ItemAxe
// Reference: apclient/minecraft/src/net/minecraft/src/ItemAxe.java
class ItemAxe : public ItemTool
{
public:
	ItemAxe(int_t id, int_t tier);
	virtual ~ItemAxe() {}
};
