#include "world/level/levelgen/feature/MonsterRoomFeature.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/MossyCobblestoneTile.h"
#include "world/level/tile/ChestTile.h"
#include "world/level/tile/MobSpawnerTile.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "world/level/material/Material.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "java/Random.h"
#include "java/String.h"
#include <memory>

// Beta 1.2 MonsterRoomFeature
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/levelgen/feature/MonsterRoomFeature.java
bool MonsterRoomFeature::place(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Beta: MonsterRoomFeature.java:15-40 - room validation
	byte_t var6 = 3;  // Height
	int_t var7 = random.nextInt(2) + 2;  // Width (2-3)
	int_t var8 = random.nextInt(2) + 2;  // Depth (2-3)
	int_t var9 = 0;  // Valid entrance count

	// Check if room can be placed (validate floor/ceiling and count entrances)
	for (int_t var10 = x - var7 - 1; var10 <= x + var7 + 1; var10++)
	{
		for (int_t var11 = y - 1; var11 <= y + var6 + 1; var11++)
		{
			for (int_t var12 = z - var8 - 1; var12 <= z + var8 + 1; var12++)
			{
				const Material &var13 = level.getMaterial(var10, var11, var12);
				if (var11 == y - 1 && !var13.isSolid())
				{
					return false;  // Floor must be solid
				}

				if (var11 == y + var6 + 1 && !var13.isSolid())
				{
					return false;  // Ceiling must be solid
				}

				// Count valid entrances (empty space on walls at floor level)
				if ((var10 == x - var7 - 1 || var10 == x + var7 + 1 || var12 == z - var8 - 1 || var12 == z + var8 + 1)
				    && var11 == y
				    && level.isEmptyTile(var10, var11, var12)
				    && level.isEmptyTile(var10, var11 + 1, var12))
				{
					var9++;
				}
			}
		}
	}

	// Beta: MonsterRoomFeature.java:42 - only place if 1-5 entrances
	if (var9 >= 1 && var9 <= 5)
	{
		// Beta: MonsterRoomFeature.java:43-64 - carve room and place walls
		for (int_t var19 = x - var7 - 1; var19 <= x + var7 + 1; var19++)
		{
			for (int_t var22 = y + var6; var22 >= y - 1; var22--)
			{
				for (int_t var24 = z - var8 - 1; var24 <= z + var8 + 1; var24++)
				{
					// Carve interior
					if (var19 != x - var7 - 1
					    && var22 != y - 1
					    && var24 != z - var8 - 1
					    && var19 != x + var7 + 1
					    && var22 != y + var6 + 1
					    && var24 != z + var8 + 1)
					{
						level.setTile(var19, var22, var24, 0);  // Air
					}
					// Remove blocks floating in air
					else if (var22 >= 0 && !level.getMaterial(var19, var22 - 1, var24).isSolid())
					{
						level.setTile(var19, var22, var24, 0);  // Air
					}
					// Place walls (cobblestone or mossy cobblestone)
					else if (level.getMaterial(var19, var22, var24).isSolid())
					{
						if (var22 == y - 1 && random.nextInt(4) != 0)
						{
							// Beta: Tile.mossStone.id (ID 48) - mossy cobblestone on floor
							level.setTile(var19, var22, var24, 48);  // Mossy cobblestone
						}
						else
						{
							// Beta: Tile.stoneBrick.id (ID 4) - regular cobblestone
							level.setTile(var19, var22, var24, 4);  // Cobblestone
						}
					}
				}
			}
		}

		// Beta: MonsterRoomFeature.java:66-102 - place chests (2 attempts, 3 tries each)
		for (int_t var20 = 0; var20 < 2; var20++)
		{
			for (int_t var23 = 0; var23 < 3; var23++)
			{
				int_t var25 = x + random.nextInt(var7 * 2 + 1) - var7;
				int_t var14 = z + random.nextInt(var8 * 2 + 1) - var8;
				if (level.isEmptyTile(var25, y, var14))
				{
					// Count solid adjacent blocks (need exactly 1 for chest placement)
					int_t var15 = 0;
					if (level.getMaterial(var25 - 1, y, var14).isSolid())
					{
						var15++;
					}

					if (level.getMaterial(var25 + 1, y, var14).isSolid())
					{
						var15++;
					}

					if (level.getMaterial(var25, y, var14 - 1).isSolid())
					{
						var15++;
					}

					if (level.getMaterial(var25, y, var14 + 1).isSolid())
					{
						var15++;
					}

					// Beta: Place chest if exactly 1 adjacent solid block
					if (var15 == 1)
					{
						// Beta: Tile.chest.id (ID 54)
						level.setTile(var25, y, var14, 54);  // Chest
						
						// Beta: Create chest tile entity and fill with loot
						Tile *chestTile = Tile::tiles[54];
						if (chestTile != nullptr)
						{
							std::shared_ptr<TileEntity> chestEntity = chestTile->newTileEntity();
							if (chestEntity != nullptr)
							{
								chestEntity->x = var25;
								chestEntity->y = y;
								chestEntity->z = var14;
								// Don't set level during world generation - it will be set when the chunk is loaded
								// Setting it here causes bad_weak_ptr because Level is passed by reference, not shared_ptr
								chestEntity->level = nullptr;
								level.setTileEntity(var25, y, var14, chestEntity);
								
								// Beta: Fill chest with loot
								// Use setItemDirect() to avoid calling setChanged() which requires level to be set
								ChestTileEntity *chest = dynamic_cast<ChestTileEntity*>(chestEntity.get());
								if (chest != nullptr)
								{
									for (int_t var17 = 0; var17 < 8; var17++)
									{
										std::shared_ptr<ItemStack> loot = randomItem(random);
										if (loot != nullptr && !loot->isEmpty())
										{
											chest->setItemDirect(random.nextInt(chest->getContainerSize()), loot);
										}
									}
								}
							}
						}
						break;  // Successfully placed chest, move to next attempt
					}
				}
			}
		}

		// Beta: MonsterRoomFeature.java:104-106 - place mob spawner at center
		// Beta: Tile.mobSpawner.id (ID 52)
		level.setTile(x, y, z, 52);  // Mob spawner
		
		// Beta: Create spawner tile entity and set mob type
		Tile *spawnerTile = Tile::tiles[52];
		if (spawnerTile != nullptr)
		{
			std::shared_ptr<TileEntity> spawnerEntity = spawnerTile->newTileEntity();
			if (spawnerEntity != nullptr)
			{
				spawnerEntity->x = x;
				spawnerEntity->y = y;
				spawnerEntity->z = z;
				// Don't set level during world generation - it will be set when the chunk is loaded
				spawnerEntity->level = nullptr;
				level.setTileEntity(x, y, z, spawnerEntity);
				
				// Beta: Set mob type
				MobSpawnerTileEntity *spawner = dynamic_cast<MobSpawnerTileEntity*>(spawnerEntity.get());
				if (spawner != nullptr)
				{
					spawner->setEntityId(randomEntityId(random));
				}
			}
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

// Beta: MonsterRoomFeature.java:113-136 - randomItem() generates loot
std::shared_ptr<ItemStack> MonsterRoomFeature::randomItem(Random &random)
{
	int_t var2 = random.nextInt(11);
	if (var2 == 0)
	{
		// Beta: Item.saddle
		if (Items::saddle != nullptr)
			return std::make_shared<ItemStack>(Items::saddle->getShiftedIndex());
		return nullptr;
	}
	else if (var2 == 1)
	{
		// Beta: Item.ironIngot, 1-4 count
		if (Items::ironIngot != nullptr)
			return std::make_shared<ItemStack>(Items::ironIngot->getShiftedIndex(), random.nextInt(4) + 1);
		return nullptr;
	}
	else if (var2 == 2)
	{
		// Beta: Item.bread
		if (Items::bread != nullptr)
			return std::make_shared<ItemStack>(Items::bread->getShiftedIndex());
		return nullptr;
	}
	else if (var2 == 3)
	{
		// Beta: Item.wheat, 1-4 count
		if (Items::wheat != nullptr)
			return std::make_shared<ItemStack>(Items::wheat->getShiftedIndex(), random.nextInt(4) + 1);
		return nullptr;
	}
	else if (var2 == 4)
	{
		// Beta: Item.sulphur (gunpowder), 1-4 count
		if (Items::sulphur != nullptr)
			return std::make_shared<ItemStack>(Items::sulphur->getShiftedIndex(), random.nextInt(4) + 1);
		return nullptr;
	}
	else if (var2 == 5)
	{
		// Beta: Item.string, 1-4 count
		if (Items::string != nullptr)
			return std::make_shared<ItemStack>(Items::string->getShiftedIndex(), random.nextInt(4) + 1);
		return nullptr;
	}
	else if (var2 == 6)
	{
		// Beta: Item.bucket_empty
		if (Items::bucketEmpty != nullptr)
			return std::make_shared<ItemStack>(Items::bucketEmpty->getShiftedIndex());
		return nullptr;
	}
	else if (var2 == 7 && random.nextInt(100) == 0)
	{
		// Beta: Item.apple_gold (1% chance)
		if (Items::appleGold != nullptr)
			return std::make_shared<ItemStack>(Items::appleGold->getShiftedIndex());
		return nullptr;
	}
	else if (var2 == 8 && random.nextInt(2) == 0)
	{
		// Beta: Item.redStone, 1-4 count (50% chance)
		if (Items::redStone != nullptr)
			return std::make_shared<ItemStack>(Items::redStone->getShiftedIndex(), random.nextInt(4) + 1);
		return nullptr;
	}
	else
	{
		// Beta: Item.record_01 or record_02 (10% chance if var2 == 9)
		if (var2 == 9 && random.nextInt(10) == 0)
		{
			if (random.nextInt(2) == 0 && Items::record01 != nullptr)
				return std::make_shared<ItemStack>(Items::record01->getShiftedIndex());
			else if (Items::record02 != nullptr)
				return std::make_shared<ItemStack>(Items::record02->getShiftedIndex());
		}
		return nullptr;
	}
}

// Beta: MonsterRoomFeature.java:138-149 - randomEntityId() generates mob type
jstring MonsterRoomFeature::randomEntityId(Random &random)
{
	int_t var2 = random.nextInt(4);
	if (var2 == 0)
	{
		return u"Skeleton";
	}
	else if (var2 == 1)
	{
		return u"Zombie";
	}
	else if (var2 == 2)
	{
		return u"Zombie";
	}
	else
	{
		return (var2 == 3) ? u"Spider" : u"";
	}
}
