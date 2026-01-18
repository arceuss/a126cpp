#pragma once

#include "java/Type.h"
#include <unordered_set>

class Level;
enum class MobCategory;
class ChunkPos;
class TilePos;
class Mob;

// newb12: MobSpawner - entity spawning system
// Reference: newb12/net/minecraft/world/level/MobSpawner.java
namespace MobSpawner
{
	// Main tick method - spawns entities in chunks around players
	// newb12: MobSpawner.tick(Level, boolean, boolean)
	// Reference: newb12/net/minecraft/world/level/MobSpawner.java:26-115
	int_t tick(Level &level, bool spawnEnemies, bool spawnFriendlies);
}
