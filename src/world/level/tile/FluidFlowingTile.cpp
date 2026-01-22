#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include <cstdlib>

// Beta 1.2: LiquidTileDynamic.java - ported 1:1
FluidFlowingTile::FluidFlowingTile(int_t id, const LiquidMaterial &material)
	: FluidTile(id, material)
{
	// Beta: LiquidTileDynamic constructor (LiquidTileDynamic.java:12-14)
	// maxCount, result, dist are initialized as member variables
}

// Beta: LiquidTileDynamic.setStatic() converts to static block (LiquidTileDynamic.java:16-21)
void FluidFlowingTile::setStatic(Level &level, int_t x, int_t y, int_t z)
{
	int_t data = level.getData(x, y, z);
	level.setTileAndDataNoUpdate(x, y, z, id + 1, data);
	level.setTilesDirty(x, y, z, x, y, z);
	level.sendTileUpdated(x, y, z);
}

// Beta: LiquidTileDynamic.tick() - main flow logic (LiquidTileDynamic.java:23-115)
void FluidFlowingTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	int_t currentDepth = getDepth(level, x, y, z);
	byte_t decayRate = 1;
	// Beta: Check if in nether (ultraWarm dimension) - for now assume overworld (not ultraWarm)
	if (&fluidMaterial == &Material::lava && (level.dimension == nullptr || !level.dimension->ultraWarm))
	{
		decayRate = 2;
	}
	
	bool shouldFlow = true;
	if (currentDepth > 0)
	{
		int_t minNeighbor = -100;
		maxCount = 0;
		minNeighbor = getHighest(level, x - 1, y, z, minNeighbor);
		minNeighbor = getHighest(level, x + 1, y, z, minNeighbor);
		minNeighbor = getHighest(level, x, y, z - 1, minNeighbor);
		minNeighbor = getHighest(level, x, y, z + 1, minNeighbor);
		int_t newLevel = minNeighbor + decayRate;
		if (newLevel >= 8 || minNeighbor < 0)
		{
			newLevel = -1;
		}
		
		if (getDepth(level, x, y + 1, z) >= 0)
		{
			int_t aboveDepth = getDepth(level, x, y + 1, z);
			if (aboveDepth >= 8)
			{
				newLevel = aboveDepth;
			}
			else
			{
				newLevel = aboveDepth + 8;
			}
		}
		
		if (maxCount >= 2 && &fluidMaterial == &Material::water)
		{
			if (level.isSolidTile(x, y - 1, z))
			{
				newLevel = 0;
			}
			else if (&level.getMaterial(x, y - 1, z) == &fluidMaterial && level.getData(x, y - 1, z) == 0)
			{
				newLevel = 0;
			}
		}
		
		if (&fluidMaterial == &Material::lava && currentDepth < 8 && newLevel < 8 && newLevel > currentDepth && random.nextInt(4) != 0)
		{
			newLevel = currentDepth;
			shouldFlow = false;
		}
		
		if (newLevel != currentDepth)
		{
			currentDepth = newLevel;
			if (newLevel < 0)
			{
				level.setTile(x, y, z, 0);
			}
			else
			{
				level.setData(x, y, z, newLevel);
				level.addToTickNextTick(x, y, z, id);
				level.updateNeighborsAt(x, y, z, id);
			}
		}
		else if (shouldFlow)
		{
			setStatic(level, x, y, z);
		}
	}
	else
	{
		setStatic(level, x, y, z);
	}
	
	if (canSpreadTo(level, x, y - 1, z))
	{
		if (currentDepth >= 8)
		{
			level.setTileAndData(x, y - 1, z, id, currentDepth);
		}
		else
		{
			level.setTileAndData(x, y - 1, z, id, currentDepth + 8);
		}
	}
	else if (currentDepth >= 0 && (currentDepth == 0 || isWaterBlocking(level, x, y - 1, z)))
	{
		bool *spreadDirs = getSpread(level, x, y, z);
		int_t spreadLevel = currentDepth + decayRate;
		if (currentDepth >= 8)
		{
			spreadLevel = 1;
		}
		
		if (spreadLevel >= 8)
		{
			return;
		}
		
		if (spreadDirs[0])
		{
			trySpreadTo(level, x - 1, y, z, spreadLevel);
		}
		
		if (spreadDirs[1])
		{
			trySpreadTo(level, x + 1, y, z, spreadLevel);
		}
		
		if (spreadDirs[2])
		{
			trySpreadTo(level, x, y, z - 1, spreadLevel);
		}
		
		if (spreadDirs[3])
		{
			trySpreadTo(level, x, y, z + 1, spreadLevel);
		}
	}
}

// Beta: LiquidTileDynamic.trySpreadTo() spreads to position (LiquidTileDynamic.java:117-130)
void FluidFlowingTile::trySpreadTo(Level &level, int_t x, int_t y, int_t z, int_t fluidLevel)
{
	if (this->canSpreadTo(level, x, y, z))
	{
		int_t blockID = level.getTile(x, y, z);
		if (blockID > 0)
		{
			if (&fluidMaterial == &Material::lava)
			{
				this->fizz(level, x, y, z);
			}
			else
			{
				Tile::tiles[blockID]->spawnResources(level, x, y, z, level.getData(x, y, z));
			}
		}
		
		level.setTileAndData(x, y, z, this->id, fluidLevel);
	}
}

// Beta: LiquidTileDynamic.getSlopeDistance() calculates distance to source (LiquidTileDynamic.java:132-172)
int_t FluidFlowingTile::getSlopeDistance(Level &level, int_t x, int_t y, int_t z, int_t distance, int_t fromDir)
{
	int_t minDist = 1000;
	
	for (int_t dir = 0; dir < 4; ++dir)
	{
		if ((dir != 0 || fromDir != 1) && (dir != 1 || fromDir != 0) && (dir != 2 || fromDir != 3) && (dir != 3 || fromDir != 2))
		{
			int_t nx = x;
			int_t nz = z;
			if (dir == 0)
				nx = x - 1;
			if (dir == 1)
				nx++;
			if (dir == 2)
				nz = z - 1;
			if (dir == 3)
				nz++;
			
			if (!isWaterBlocking(level, nx, y, nz) &&
			    (&level.getMaterial(nx, y, nz) != &fluidMaterial || level.getData(nx, y, nz) != 0))
			{
				if (!isWaterBlocking(level, nx, y - 1, nz))
				{
					return distance;
				}
				
				if (distance < 4)
				{
					int_t dist = getSlopeDistance(level, nx, y, nz, distance + 1, dir);
					if (dist < minDist)
					{
						minDist = dist;
					}
				}
			}
		}
	}
	
	return minDist;
}

// Beta: LiquidTileDynamic.getSpread() calculates spread directions (LiquidTileDynamic.java:174-217)
bool *FluidFlowingTile::getSpread(Level &level, int_t x, int_t y, int_t z)
{
	for (int_t dir = 0; dir < 4; ++dir)
	{
		dist[dir] = 1000;
		int_t nx = x;
		int_t nz = z;
		if (dir == 0)
			nx = x - 1;
		if (dir == 1)
			nx++;
		if (dir == 2)
			nz = z - 1;
		if (dir == 3)
			nz++;
		
		if (!isWaterBlocking(level, nx, y, nz) &&
		    (&level.getMaterial(nx, y, nz) != &fluidMaterial || level.getData(nx, y, nz) != 0))
		{
			if (!isWaterBlocking(level, nx, y - 1, nz))
			{
				dist[dir] = 0;
			}
			else
			{
				dist[dir] = getSlopeDistance(level, nx, y, nz, 1, dir);
			}
		}
	}
	
	int_t minDist = dist[0];
	for (int_t i = 1; i < 4; ++i)
	{
		if (dist[i] < minDist)
		{
			minDist = dist[i];
		}
	}
	
	for (int_t i = 0; i < 4; ++i)
	{
		result[i] = (dist[i] == minDist);
	}
	
	return result;
}

// Beta: LiquidTileDynamic.isWaterBlocking() checks if block blocks water (LiquidTileDynamic.java:219-229)
bool FluidFlowingTile::isWaterBlocking(Level &level, int_t x, int_t y, int_t z)
{
	int_t blockID = level.getTile(x, y, z);
	// Beta: Check for door_wood, door_iron, sign, ladder, reeds (Tile IDs not yet defined, using hardcoded IDs)
	if (blockID == 64 || blockID == 71 || blockID == 63 || blockID == 65 || blockID == 83)
	{
		return true;
	}
	else if (blockID == 0)
	{
		return false;
	}
	else
	{
		const Material &mat = Tile::tiles[blockID]->material;
		return mat.isSolid();
	}
}

// Beta: LiquidTileDynamic.getHighest() gets highest neighbor level (LiquidTileDynamic.java:231-246)
int_t FluidFlowingTile::getHighest(Level &level, int_t x, int_t y, int_t z, int_t currentMin)
{
	int_t depth = getDepth(level, x, y, z);
	if (depth < 0)
	{
		return currentMin;
	}
	else
	{
		if (depth == 0)
		{
			maxCount++;
		}
		
		if (depth >= 8)
		{
			depth = 0;
		}
		
		return (currentMin >= 0 && depth >= currentMin) ? currentMin : depth;
	}
}

// Beta: LiquidTileDynamic.canSpreadTo() checks if can spread (LiquidTileDynamic.java:248-255)
bool FluidFlowingTile::canSpreadTo(Level &level, int_t x, int_t y, int_t z)
{
	const Material &mat = level.getMaterial(x, y, z);
	if (&mat == &fluidMaterial)
	{
		return false;
	}
	else
	{
		return (&mat == &Material::lava) ? false : !isWaterBlocking(level, x, y, z);
	}
}

// Beta: LiquidTileDynamic.onPlace() schedules update (LiquidTileDynamic.java:257-263)
void FluidFlowingTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	FluidTile::onPlace(level, x, y, z);
	if (level.getTile(x, y, z) == id)
	{
		level.addToTickNextTick(x, y, z, id);
	}
}
