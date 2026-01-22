#pragma once

#include "world/level/levelgen/feature/Feature.h"
#include "java/Type.h"
#include "java/String.h"
#include "world/item/ItemStack.h"
#include <memory>

class Level;
class Random;

// Beta 1.2 MonsterRoomFeature
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/levelgen/feature/MonsterRoomFeature.java
class MonsterRoomFeature : public Feature
{
public:
	bool place(Level &level, Random &random, int_t x, int_t y, int_t z) override;

private:
	// Beta: randomItem() - generates random loot item (MonsterRoomFeature.java:113-136)
	// Returns nullptr if item doesn't exist (TODO: implement items)
	std::shared_ptr<ItemStack> randomItem(Random &random);
	
	// Beta: randomEntityId() - generates random mob type for spawner (MonsterRoomFeature.java:138-149)
	jstring randomEntityId(Random &random);
};
