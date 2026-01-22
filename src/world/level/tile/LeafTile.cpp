#include "world/level/tile/LeafTile.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/FoliageColor.h"
#include "world/level/tile/SaplingTile.h"
#include "java/Random.h"
#include "world/entity/Entity.h"

// Static member initialization
int_t *LeafTile::checkBuffer = nullptr;

LeafTile::LeafTile(int_t id, int_t tex) : TransparentTile(id, tex, Material::leaves, false)  // Beta: Material.leaves (LeafTile.java:21)
{
	oTex = tex;
	setTicking(true);
}

int_t LeafTile::getColor(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Alpha 1.2.6: BlockLeaves.colorMultiplier() - only uses biome temperature/humidity
	// No leaf type variants - only oak leaves exist
	// Alpha: var1.func_4075_a().func_4069_a(var2, var4, 1, 1);
	//        double var5 = var1.func_4075_a().temperature[0];
	//        double var7 = var1.func_4075_a().humidity[0];
	//        return ColorizerFoliage.func_4146_a(var5, var7);
	level.getBiomeSource().getBiomeBlock(x, z, 1, 1);
	double temperature = level.getBiomeSource().temperatures[0];
	double downfall = level.getBiomeSource().downfalls[0];

	return FoliageColor::get(temperature, downfall);
}

void LeafTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeafTile.onRemove() - sets UPDATE_LEAF_BIT on nearby leaves (LeafTile.java:42-58)
	byte_t range = 1;
	int_t checkRange = range + 1;
	if (level.hasChunksAt(x - checkRange, y - checkRange, z - checkRange, x + checkRange, y + checkRange, z + checkRange))
	{
		for (int_t dx = -range; dx <= range; dx++)
		{
			for (int_t dy = -range; dy <= range; dy++)
			{
				for (int_t dz = -range; dz <= range; dz++)
				{
					int_t tileId = level.getTile(x + dx, y + dy, z + dz);
					if (tileId == Tile::leaves.id)
					{
						int_t data = level.getData(x + dx, y + dy, z + dz);
						level.setDataNoUpdate(x + dx, y + dy, z + dz, data | UPDATE_LEAF_BIT);
					}
				}
			}
		}
	}
}

void LeafTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: LeafTile.tick() - complex decay logic (LeafTile.java:61-133)
	if (!level.isOnline)
	{
		int_t data = level.getData(x, y, z);
		if ((data & UPDATE_LEAF_BIT) != 0)
		{
			byte_t range = REQUIRED_WOOD_RANGE;
			int_t checkRange = range + 1;
			byte_t bufferSize = 32;
			int_t bufferVolume = bufferSize * bufferSize;
			int_t bufferCenter = bufferSize / 2;
			
			// Allocate checkBuffer if needed
			if (checkBuffer == nullptr)
			{
				checkBuffer = new int_t[bufferSize * bufferSize * bufferSize];
			}
			
			if (level.hasChunksAt(x - checkRange, y - checkRange, z - checkRange, x + checkRange, y + checkRange, z + checkRange))
			{
				// Initialize checkBuffer
				for (int_t dx = -range; dx <= range; dx++)
				{
					for (int_t dy = -range; dy <= range; dy++)
					{
						for (int_t dz = -range; dz <= range; dz++)
						{
							int_t tileId = level.getTile(x + dx, y + dy, z + dz);
							int_t idx = (dx + bufferCenter) * bufferVolume + (dy + bufferCenter) * bufferSize + (dz + bufferCenter);
							if (tileId == Tile::treeTrunk.id)
							{
								checkBuffer[idx] = 0;
							}
							else if (tileId == Tile::leaves.id)
							{
								checkBuffer[idx] = -2;
							}
							else
							{
								checkBuffer[idx] = -1;
							}
						}
					}
				}
				
				// Propagate distances from wood
				for (int_t distance = 1; distance <= REQUIRED_WOOD_RANGE; distance++)
				{
					for (int_t dx = -range; dx <= range; dx++)
					{
						for (int_t dy = -range; dy <= range; dy++)
						{
							for (int_t dz = -range; dz <= range; dz++)
							{
								int_t idx = (dx + bufferCenter) * bufferVolume + (dy + bufferCenter) * bufferSize + (dz + bufferCenter);
								if (checkBuffer[idx] == distance - 1)
								{
									// Check all 6 neighbors
									int_t neighbors[6][3] = {{-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}};
									for (int_t n = 0; n < 6; n++)
									{
										int_t nIdx = (dx + neighbors[n][0] + bufferCenter) * bufferVolume + 
										              (dy + neighbors[n][1] + bufferCenter) * bufferSize + 
										              (dz + neighbors[n][2] + bufferCenter);
										if (checkBuffer[nIdx] == -2)
										{
											checkBuffer[nIdx] = distance;
										}
									}
								}
							}
						}
					}
				}
				
				// Check if this leaf is connected to wood
				int_t centerIdx = bufferCenter * bufferVolume + bufferCenter * bufferSize + bufferCenter;
				if (checkBuffer[centerIdx] >= 0)
				{
					// Connected to wood - clear UPDATE_LEAF_BIT
					level.setData(x, y, z, data & ~UPDATE_LEAF_BIT);
				}
				else
				{
					// Not connected - die
					die(level, x, y, z);
				}
			}
		}
	}
}

void LeafTile::die(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: LeafTile.die() - spawns resources and removes block (LeafTile.java:135-138)
	spawnResources(level, x, y, z, level.getData(x, y, z));
	level.setTile(x, y, z, 0);
}

int_t LeafTile::getResourceCount(Random &random)
{
	// Alpha: BlockLeaves.quantityDropped() - 1/20 chance (5%) for sapling (BlockLeaves.java:108-110)
	// Beta uses 1/16, but Alpha uses 1/20
	return (random.nextInt(20) == 0) ? 1 : 0;
}

int_t LeafTile::getResource(int_t data, Random &random)
{
	// Beta: LeafTile.getResource() returns Tile.sapling.id (LeafTile.java:146-148)
	return Tile::sapling.id;
}

bool LeafTile::isSolidRender()
{
	return !allowSame;
}

int_t LeafTile::getTexture(Facing face, int_t data)
{
	// Alpha 1.2.6: BlockLeaves.getBlockTextureFromSide() is inherited from Block
	// Only uses baseIndexInPNG (tex) which changes with setGraphicsLevel() (fancy/non-fancy)
	// No type variants - only oak leaves exist, metadata is only for decay checking
	// Alpha: setGraphicsLevel() sets blockIndexInTexture = baseIndexInPNG + (var1 ? 0 : 1)
	return tex;
}

void LeafTile::setFancy(bool fancy)
{
	allowSame = fancy;
	tex = oTex + (fancy ? 0 : 1);
}

void LeafTile::stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	Tile::stepOn(level, x, y, z, entity);
}
