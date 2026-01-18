#pragma once

#include "world/item/ItemStack.h"

class CraftingContainer;

// Beta: Recipy interface (Recipy.java:6-12)
class Recipy
{
public:
	virtual ~Recipy() {}
	
	// Beta: Recipy interface methods
	virtual bool matches(CraftingContainer &container) = 0;
	virtual ItemStack assemble(CraftingContainer &container) = 0;
	virtual int_t size() = 0;
};