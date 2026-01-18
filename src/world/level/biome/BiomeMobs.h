#pragma once

#include "world/level/biome/BiomeSource.h"
#include "world/entity/MobCategory.h"
#include <vector>
#include <functional>
#include <memory>

class Level;
class Mob;

// newb12: Biome entity mapping system
// Reference: newb12/net/minecraft/world/level/biome/Biome.java:128-136
// Note: In newb12, all biomes have the same entity lists, so we use a simple mapping
// Since C++ doesn't have reflection, we use factory functions instead of Class arrays

namespace BiomeMobs
{
	// Entity factory function type
	using EntityFactory = std::function<std::shared_ptr<Mob>(Level&)>;

	// Get entity factories for a MobCategory
	// newb12: All biomes have the same entity lists:
	//   enemies: Spider, Zombie, Skeleton, Creeper
	//   friendlies: Sheep, Pig, Chicken, Cow
	//   waterFriendlies: Squid (excluded - not in Alpha)
	// Reference: newb12/net/minecraft/world/level/biome/Biome.java:39-41
	const std::vector<EntityFactory> &getMobs(MobCategory category);
}
