#pragma once

#include "world/level/biome/BiomeSource.h"
#include "world/entity/MobCategory.h"
#include <vector>
#include <functional>
#include <memory>

class Level;
class Mob;

// Alpha: MobSpawnerBase holds a monster and a creature class array per biome and
// hands the requested category to the spawner (MobSpawnerBase.java:39-48,131-138).
// MobSpawnerHell replaces both: ghasts and pig zombies, and no animals at all
// (MobSpawnerHell.java:12-15).
// C++ has no reflection, so factory functions stand in for the Class arrays.

namespace BiomeMobs
{
	// Entity factory function type
	using EntityFactory = std::function<std::shared_ptr<Mob>(Level&)>;

	// Alpha: MobSpawnerBase.getEntitiesForType() (MobSpawnerBase.java:131-138)
	const std::vector<EntityFactory> &getMobs(BiomeType biome, MobCategory category);
}
