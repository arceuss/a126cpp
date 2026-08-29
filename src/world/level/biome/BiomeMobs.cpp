#include "world/level/biome/BiomeMobs.h"

#include "world/entity/MobCategory.h"
#include "world/entity/Mob.h"
#include "world/entity/animal/Chicken.h"
#include "world/entity/animal/Cow.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/animal/Sheep.h"
#include "world/entity/monster/Creeper.h"
#include "world/entity/monster/Skeleton.h"
#include "world/entity/monster/Spider.h"
#include "world/entity/monster/Zombie.h"
#include "world/entity/monster/Ghast.h"
#include "world/entity/monster/PigZombie.h"
#include "world/level/Level.h"
#include "util/Memory.h"
#include <memory>
#include <vector>

// Direct replacement for Alpha MobSpawnerBase's class arrays
// (MobSpawnerBase.java:43-49).

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

	static std::vector<EntityFactory> monsterFactories = {
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Spider>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Zombie>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Skeleton>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Creeper>(level);
		}
	};

	// Alpha: MobSpawnerHell.biomeMonsters = {Ghast, PigZombie} (MobSpawnerHell.java:13)
	static std::vector<EntityFactory> hellMonsterFactories = {
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<Ghast>(level);
		},
		[](Level &level) -> std::shared_ptr<Mob> {
			return Util::make_shared<PigZombie>(level);
		}
	};

	// Entity factories for waterCreature category (excluded - Squid not in Alpha)
	// newb12: waterFriendlies = {Squid.class}
	// Reference: newb12/net/minecraft/world/level/biome/Biome.java:41
	static std::vector<EntityFactory> waterCreatureFactories = {
		// Excluded - Squid not in Alpha
	};

	// Alpha: MobSpawnerHell.biomeCreatures = new Class[0] (MobSpawnerHell.java:14)
	static std::vector<EntityFactory> noFactories;

	const std::vector<EntityFactory> &getMobs(BiomeType biome, MobCategory category)
	{
		if (biome == BiomeType::HELL)
		{
			switch (category)
			{
			case MobCategory::monster:
				return hellMonsterFactories;
			default:
				return noFactories;
			}
		}

		switch (category)
		{
		case MobCategory::monster:
			return monsterFactories;
		case MobCategory::creature:
			return creatureFactories;
		case MobCategory::waterCreature:
			return waterCreatureFactories;
		default:
			return noFactories;
		}
	}
}
