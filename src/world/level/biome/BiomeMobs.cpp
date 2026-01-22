#include "world/level/biome/BiomeMobs.h"

#include "world/entity/MobCategory.h"
#include "world/entity/Mob.h"
#include "world/entity/animal/Chicken.h"
#include "world/entity/animal/Cow.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/animal/Sheep.h"
#include "world/level/Level.h"
#include "util/Memory.h"
#include <memory>
#include <vector>

// newb12: Biome entity mapping system
// Reference: newb12/net/minecraft/world/level/biome/Biome.java:128-136
// Note: Monsters (Spider, Zombie, Skeleton, Creeper) are not yet implemented
// Only animals (Chicken, Cow, Pig, Sheep) are available for now

namespace BiomeMobs
{
	// Entity factories for creature category (animals)
	// newb12: friendlies = {Sheep.class, Pig.class, Chicken.class, Cow.class}
	// Reference: newb12/net/minecraft/world/level/biome/Biome.java:40
	static std::vector<EntityFactory> creatureFactories = {
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Sheep>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Pig>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Chicken>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Cow>(level);
		}
	};

	// Entity factories for monster category (not yet implemented)
	// newb12: enemies = {Spider.class, Zombie.class, Skeleton.class, Creeper.class}
	// Reference: newb12/net/minecraft/world/level/biome/Biome.java:39
	static std::vector<EntityFactory> monsterFactories = {
		// TODO: Add when monsters are implemented
		// Spider, Zombie, Skeleton, Creeper
	};

	// Entity factories for waterCreature category (excluded - Squid not in Alpha)
	// newb12: waterFriendlies = {Squid.class}
	// Reference: newb12/net/minecraft/world/level/biome/Biome.java:41
	static std::vector<EntityFactory> waterCreatureFactories = {
		// Excluded - Squid not in Alpha
	};

	const std::vector<EntityFactory> &getMobs(MobCategory category)
	{
		switch (category)
		{
		case MobCategory::monster:
			return monsterFactories;
		case MobCategory::creature:
			return creatureFactories;
		case MobCategory::waterCreature:
			return waterCreatureFactories;
		default:
			// Return empty vector for unknown categories
			static std::vector<EntityFactory> empty;
			return empty;
		}
	}
}
