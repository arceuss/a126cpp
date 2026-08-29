#include "world/level/MobSpawner.h"

#include "world/level/Level.h"
#include "world/level/ChunkPos.h"
#include "world/level/TilePos.h"
#include "world/level/biome/BiomeMobs.h"
#include "world/level/biome/BiomeSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/entity/MobCategory.h"
#include "world/entity/Mob.h"
#include "world/entity/monster/Skeleton.h"
#include "world/entity/monster/Spider.h"
#include "util/Mth.h"
#include <unordered_set>
#include <vector>
#include <iostream>

// newb12: MobSpawner - entity spawning system
// Reference: newb12/net/minecraft/world/level/MobSpawner.java

namespace MobSpawner
{
	static constexpr int_t MIN_SPAWN_DISTANCE = 24;
	static std::unordered_set<ChunkPos> chunksToPoll;

	// newb12: MobSpawner.getRandomPosWithin() - gets random position within chunk
	// Reference: newb12/net/minecraft/world/level/MobSpawner.java:19-24
	static TilePos getRandomPosWithin(Level &level, int_t x, int_t z)
	{
		int_t px = x + level.random.nextInt(16);
		int_t py = level.random.nextInt(128);
		int_t pz = z + level.random.nextInt(16);
		return TilePos(px, py, pz);
	}

	// newb12: MobSpawner.isSpawnPositionOk() - checks if position is valid for spawning
	// Reference: newb12/net/minecraft/world/level/MobSpawner.java:117-124
	static bool isSpawnPositionOk(MobCategory category, Level &level, int_t x, int_t y, int_t z)
	{
		const Material &spawnMaterial = MobCategoryHelper::getSpawnPositionMaterial(category);
		if (spawnMaterial == Material::water)
		{
			return level.getMaterial(x, y, z).isLiquid() && !level.isSolidTile(x, y + 1, z);
		}
		else
		{
			return level.isSolidTile(x, y - 1, z)
				&& !level.isSolidTile(x, y, z)
				&& !level.getMaterial(x, y, z).isLiquid()
				&& !level.isSolidTile(x, y + 1, z);
		}
	}

	static void finalizeMobSettings(std::shared_ptr<Mob> mob, Level &level,
		float x, float y, float z)
	{
		// Direct Alpha transliteration: SpawnerAnimals.java:85-90.
		std::shared_ptr<Spider> spider = std::dynamic_pointer_cast<Spider>(mob);
		if (spider != nullptr && level.random.nextInt(100) == 0)
		{
			std::shared_ptr<Skeleton> skeleton = std::make_shared<Skeleton>(level);
			skeleton->moveTo(x, y, z, mob->yRot, 0.0f);
			level.addEntity(skeleton);
			skeleton->mountEntity(mob.get());
		}
	}

	// newb12: MobSpawner.tick() - main spawning logic
	// Reference: newb12/net/minecraft/world/level/MobSpawner.java:26-115
	int_t tick(Level &level, bool spawnEnemies, bool spawnFriendlies)
	{
		if (!spawnEnemies && !spawnFriendlies)
		{
			return 0;
		}
		else
		{
			chunksToPoll.clear();

			for (size_t i = 0; i < level.players.size(); i++)
			{
				auto &player = level.players[i];
				int_t cx = Mth::floor(player->x / 16.0);
				int_t cz = Mth::floor(player->z / 16.0);
				int_t radius = 8;

				for (int_t dx = -radius; dx <= radius; dx++)
				{
					for (int_t dz = -radius; dz <= radius; dz++)
					{
						chunksToPoll.insert(ChunkPos(dx + cx, dz + cz));
					}
				}
			}

			int_t totalSpawned = 0;

			// Iterate over all MobCategory values
			MobCategory categories[] = { MobCategory::monster, MobCategory::creature, MobCategory::waterCreature };
			for (MobCategory category : categories)
			{
				int_t currentCount = level.countInstanceOf(category);
				int_t maxCount = MobCategoryHelper::getMaxInstancesPerChunk(category) * chunksToPoll.size() / 256;
				bool categoryAllowed = (!MobCategoryHelper::isFriendly(category) || spawnFriendlies)
					&& (MobCategoryHelper::isFriendly(category) || spawnEnemies);
				
				if (categoryAllowed && currentCount <= maxCount)
				{
					label109:
					for (const ChunkPos &chunkPos : chunksToPoll)
					{
						// Direct Alpha admission gate: SpawnerAnimals.java:48-52.
						// It occurs before class selection and position RNG.
						if (level.random.nextInt(50) != 0)
							continue;
						// Get biome at chunk center (convert chunk coordinates to world coordinates)
						BiomeSource &biomeSource = level.getBiomeSource();
						int_t worldX = chunkPos.x * 16 + 8;
						int_t worldZ = chunkPos.z * 16 + 8;
						BiomeType biome = biomeSource.getBiomeAt(worldX, worldZ);

						// Alpha: the biome at the chunk decides which classes may
						// spawn (SpawnerAnimals.java:57, MobSpawnerBase.java:131-138)
						const auto &entityFactories = BiomeMobs::getMobs(biome, category);
						if (!entityFactories.empty())
						{
							int_t factoryIndex = level.random.nextInt(static_cast<int_t>(entityFactories.size()));
							TilePos spawnPos = getRandomPosWithin(level, chunkPos.x * 16, chunkPos.z * 16);
							int_t x = spawnPos.x;
							int_t y = spawnPos.y;
							int_t z = spawnPos.z;

							if (!level.isSolidTile(x, y, z) && level.getMaterial(x, y, z) == MobCategoryHelper::getSpawnPositionMaterial(category))
							{
								int_t clusterSize = 0;

								for (int_t attempt = 0; attempt < 3; attempt++)
								{
									int_t px = x;
									int_t py = y;
									int_t pz = z;
									int_t spread = 6;

									for (int_t offset = 0; offset < 4; offset++)
									{
										px += level.random.nextInt(spread) - level.random.nextInt(spread);
										py += level.random.nextInt(1) - level.random.nextInt(1);
										pz += level.random.nextInt(spread) - level.random.nextInt(spread);

										if (isSpawnPositionOk(category, level, px, py, pz))
										{
											float fx = px + 0.5f;
											float fy = py;
											float fz = pz + 0.5f;

											if (level.getNearestPlayer(fx, fy, fz, 24.0) == nullptr)
											{
												float dx = fx - level.xSpawn;
												float dy = fy - level.ySpawn;
												float dz = fz - level.zSpawn;
												float distSqr = dx * dx + dy * dy + dz * dz;

												if (!(distSqr < 576.0f))
												{
													// Create entity using factory (replaces Java reflection)
													std::shared_ptr<Mob> mob = entityFactories[factoryIndex](level);

													mob->moveTo(fx, fy, fz, level.random.nextFloat() * 360.0f, 0.0f);
													if (mob->canSpawn())
													{
														clusterSize++;
														level.addEntity(mob);
														finalizeMobSettings(mob, level, fx, fy, fz);
														if (clusterSize >= mob->getMaxSpawnClusterSize())
														{
															totalSpawned += clusterSize;
															goto label109;
														}
													}

													// newb12: var32 += var17; (line 101)
													// Accumulate clusterSize after each spawn attempt
													totalSpawned += clusterSize;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			return totalSpawned;
		}
	}
}
