#pragma once

#include "java/Type.h"
#include "world/level/material/Material.h"
#include <functional>
#include <memory>

class Level;
class Mob;

// newb12: MobCategory - entity spawning categories
// Reference: newb12/net/minecraft/world/entity/MobCategory.java
enum class MobCategory
{
	monster,      // Enemy.class, 70 max, Material.air, false (not friendly)
	creature,     // Animal.class, 15 max, Material.air, true (friendly)
	waterCreature // WaterAnimal.class, 5 max, Material.water, true (friendly)
};

namespace MobCategoryHelper
{
	// Get base class type identifier (for countInstanceOf)
	int_t getBaseClassType(MobCategory category);

	// Get max instances per chunk
	int_t getMaxInstancesPerChunk(MobCategory category);

	// Get spawn position material
	const Material &getSpawnPositionMaterial(MobCategory category);

	// Check if category is friendly
	bool isFriendly(MobCategory category);
}